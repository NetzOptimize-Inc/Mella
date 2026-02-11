import WifiManager from 'react-native-wifi-reborn';
import NetInfo from '@react-native-community/netinfo';
import {Platform, PermissionsAndroid} from 'react-native';

/**
 * Request location permission (required for WiFi operations on Android)
 * @returns True if permission granted
 */
const requestLocationPermission = async (): Promise<boolean> => {
  if (Platform.OS !== 'android') {
    return true; // iOS doesn't need this permission
  }

  try {
    const granted = await PermissionsAndroid.request(
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      {
        title: 'Location Permission',
        message: 'This app needs access to your location to connect to Wi-Fi networks.',
        buttonNeutral: 'Ask Me Later',
        buttonNegative: 'Cancel',
        buttonPositive: 'OK',
      },
    );
    return granted === PermissionsAndroid.RESULTS.GRANTED;
  } catch (err) {
    console.warn('Permission request error:', err);
    return false;
  }
};

/**
 * Connect to a WiFi network
 * @param ssid - WiFi network SSID
 * @param password - WiFi network password (optional for open networks)
 * @returns True if connection successful
 */
export const connectToWifi = async (
  ssid: string,
  password: string | null = null,
): Promise<boolean> => {
  try {
    console.log(`Attempting to connect to WiFi: ${ssid}`);

    // Check if already connected to this network
    const currentSSID = await WifiManager.getCurrentWifiSSID();
    if (currentSSID && currentSSID.replace(/"/g, '') === ssid) {
      console.log('Already connected to this network');
      return true;
    }

    // Request location permission
    const hasPermission = await requestLocationPermission();
    if (!hasPermission) {
      throw new Error(
        'Location permission denied. WiFi connection requires location permission.',
      );
    }

    // For open networks, try multiple connection methods
    if (!password) {
      console.log('Attempting to connect to open network:', ssid);
      
      let connectionInitiated = false;
      let lastError: any = null;
      
      // Method 1: Try connectToProtectedSSID with empty string
      try {
        await WifiManager.connectToProtectedSSID(ssid, '', false);
        console.log('Connection request sent (method 1: empty string), waiting...');
        connectionInitiated = true;
      } catch (connectError: any) {
        console.warn('Method 1 failed:', connectError?.message || connectError);
        lastError = connectError;
        
        // Method 2: Try with 'nopass' (some libraries use this)
        try {
          await WifiManager.connectToProtectedSSID(ssid, 'nopass', false);
          console.log('Connection request sent (method 2: nopass), waiting...');
          connectionInitiated = true;
        } catch (retryError: any) {
          console.warn('Method 2 failed:', retryError?.message || retryError);
          lastError = retryError;
          
          // Method 3: Try with null (might work in some versions)
          try {
            await WifiManager.connectToProtectedSSID(ssid, null as any, false);
            console.log('Connection request sent (method 3: null), waiting...');
            connectionInitiated = true;
          } catch (finalError: any) {
            console.warn('Method 3 failed:', finalError?.message || finalError);
            lastError = finalError;
          }
        }
      }
      
      if (!connectionInitiated) {
        const errorMsg = lastError?.message || 'Unknown error';
        console.error('All connection methods failed. Last error:', errorMsg);
        throw new Error(`Failed to initiate WiFi connection: ${errorMsg}\n\nNote: Android may restrict automatic WiFi connections. Please connect manually via WiFi settings.`);
      }
    } else {
      // For protected networks
      await WifiManager.connectToProtectedSSID(ssid, password, false);
    }

    // Wait longer for connection to establish (open networks may take longer)
    console.log('Waiting for connection to establish...');
    await new Promise(resolve => setTimeout(resolve, 5000));

    // Verify connection with multiple attempts
    let connected = false;
    for (let attempt = 0; attempt < 5; attempt++) {
      const connectedSSID = await WifiManager.getCurrentWifiSSID();
      const verifiedSSID = connectedSSID ? connectedSSID.replace(/"/g, '') : '';
      
      if (verifiedSSID === ssid) {
        console.log(`Successfully connected to WiFi: ${ssid} (attempt ${attempt + 1})`);
        connected = true;
        break;
      }
      
      if (attempt < 4) {
        console.log(`Connection not yet established, waiting... (attempt ${attempt + 1}/5)`);
        await new Promise(resolve => setTimeout(resolve, 2000));
      }
    }

    if (connected) {
      return true;
    } else {
      // Final check
      const finalSSID = await WifiManager.getCurrentWifiSSID();
      const finalVerifiedSSID = finalSSID ? finalSSID.replace(/"/g, '') : '';
      throw new Error(`Connection failed. Currently connected to: ${finalVerifiedSSID || 'none'}`);
    }
  } catch (error: any) {
    console.error('WiFi connection error:', error);
    throw error;
  }
};

/**
 * Check if device is connected to a specific WiFi network
 * @param ssid - WiFi network SSID to check
 * @returns True if connected to the specified network
 */
export const isConnectedToWifi = async (ssid: string): Promise<boolean> => {
  try {
    const currentSSID = await WifiManager.getCurrentWifiSSID();
    if (!currentSSID) {
      return false;
    }
    const cleanedSSID = currentSSID.replace(/"/g, '');
    return cleanedSSID === ssid;
  } catch (error) {
    console.error('Error checking WiFi connection:', error);
    return false;
  }
};

/**
 * Check network connectivity and IP address
 */
export const checkNetworkStatus = async (): Promise<{
  isConnected: boolean;
  isInternetReachable: boolean;
  type: string;
  ipAddress: string;
}> => {
  try {
    const state = await NetInfo.fetch();
    const ipAddress = (state.details as any)?.ipAddress || 'Unknown';
    
    return {
      isConnected: state.isConnected ?? false,
      isInternetReachable: state.isInternetReachable ?? false,
      type: state.type,
      ipAddress: ipAddress,
    };
  } catch (error) {
    console.error('Error checking network status:', error);
    return {
      isConnected: false,
      isInternetReachable: false,
      type: 'unknown',
      ipAddress: 'Unknown',
    };
  }
};

/**
 * Wait for connection to ESP32 AP mode network
 * @param ssid - ESP32 AP mode SSID
 * @param maxAttempts - Maximum connection attempts
 * @param delayMs - Delay between attempts in milliseconds
 * @returns True if connection successful
 */
export const waitForEsp32Connection = async (
  ssid: string,
  maxAttempts: number = 10,
  delayMs: number = 2000,
): Promise<boolean> => {
  for (let i = 0; i < maxAttempts; i++) {
    const connected = await isConnectedToWifi(ssid);
    if (connected) {
      // Wait a bit more for IP assignment
      await new Promise(resolve => setTimeout(resolve, 2000));
      return true;
    }
    console.log(`Connection attempt ${i + 1}/${maxAttempts}...`);
    await new Promise(resolve => setTimeout(resolve, delayMs));
  }
  return false;
};

