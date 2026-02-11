import React, {useState, useEffect} from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  StyleSheet,
  ActivityIndicator,
  Alert,
  Platform,
  PermissionsAndroid,
} from 'react-native';
import WifiManager from 'react-native-wifi-reborn';

interface WiFiNetwork {
  SSID: string;
  capabilities?: string;
  level?: number;
}

interface WifiScannerProps {
  onNetworkSelected: (network: WiFiNetwork) => void;
  onCancel: () => void;
}

const WifiScanner: React.FC<WifiScannerProps> = ({onNetworkSelected, onCancel}) => {
  const [wifiNetworks, setWifiNetworks] = useState<WiFiNetwork[]>([]);
  const [loading, setLoading] = useState(false);
  const [scanning, setScanning] = useState(false);

  useEffect(() => {
    scanForNetworks();
  }, []);

  const requestLocationPermission = async (): Promise<boolean> => {
    if (Platform.OS !== 'android') {
      return true; // iOS doesn't need this permission for WiFi scanning
    }

    try {
      const granted = await PermissionsAndroid.request(
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
        {
          title: 'Location Permission',
          message: 'This app needs access to your location to scan for Wi-Fi networks.',
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

  const scanForNetworks = async () => {
    try {
      setScanning(true);
      setLoading(true);
      
      // Request location permission (required for WiFi scanning on Android)
      const hasPermission = await requestLocationPermission();
      
      if (!hasPermission) {
        Alert.alert(
          'Permission Required',
          'Location permission is required to scan WiFi networks. Please enable it in app settings.',
        );
        setLoading(false);
        setScanning(false);
        return;
      }

      // Force a WiFi scan first (this may not be available in all versions)
      try {
        console.log('Requesting WiFi scan...');
        // Some versions have reScanAndLoadWifiList or forceWifiUsage
        if (WifiManager.reScanAndLoadWifiList) {
          await WifiManager.reScanAndLoadWifiList();
          console.log('WiFi scan completed');
        } else {
          console.log('reScanAndLoadWifiList not available, using loadWifiList');
        }
      } catch (scanError: any) {
        console.warn('Force scan failed, continuing with cached list:', scanError);
      }

      // Wait a moment for scan to complete
      await new Promise<void>(resolve => setTimeout(resolve, 2000));

      // Load available WiFi networks
      console.log('Loading WiFi list...');
      const networks = await WifiManager.loadWifiList();
      console.log(`Found ${networks?.length || 0} networks`);
      
      // Filter out networks with empty SSID and show all available networks
      const allNetworks = networks.filter((network: WiFiNetwork) => {
        const hasSSID = network.SSID && network.SSID.trim().length > 0;
        if (hasSSID) {
          console.log(`Network found: ${network.SSID} (Signal: ${network.level}dBm)`);
        }
        return hasSSID;
      });
      
      console.log(`Filtered to ${allNetworks.length} valid networks`);
      
      if (allNetworks.length === 0) {
        Alert.alert(
          'No Networks Found',
          'No WiFi networks were found. Please try scanning again.',
          [
            {text: 'Scan Again', onPress: scanForNetworks},
            {text: 'Cancel', onPress: onCancel},
          ],
        );
      }
      
      setWifiNetworks(allNetworks);
    } catch (error: any) {
      console.error('WiFi scan error:', error);
      Alert.alert(
        'Scan Error',
        `Failed to scan WiFi networks: ${error.message}`,
        [
          {text: 'Retry', onPress: scanForNetworks},
          {text: 'Cancel', onPress: onCancel},
        ],
      );
    } finally {
      setLoading(false);
      setScanning(false);
    }
  };

  const handleNetworkSelect = (network: WiFiNetwork) => {
    onNetworkSelected(network);
  };

  const renderNetworkItem = ({item}: {item: WiFiNetwork}) => {
    const isDeviceNetwork = item.SSID && (
      item.SSID.startsWith('Safety') || 
      item.SSID.startsWith('Amazon')
    );
    
    return (
      <TouchableOpacity
        style={[
          styles.networkItem,
          isDeviceNetwork && styles.deviceNetworkItem
        ]}
        onPress={() => handleNetworkSelect(item)}>
        <View style={styles.networkInfo}>
          <View style={styles.networkNameRow}>
            <Text style={styles.networkName}>{item.SSID || 'Unknown'}</Text>
            {isDeviceNetwork && (
              <Text style={[styles.deviceBadge, {color: '#fff'}]}>Device</Text>
            )}
          </View>
          <Text style={styles.networkDetails}>
            Security: {item.capabilities || 'Open'} | Signal: {item.level || 'N/A'} dBm
          </Text>
        </View>
        <Text style={styles.selectArrow}>→</Text>
      </TouchableOpacity>
    );
  };

  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Select WiFi Network</Text>
        <Text style={styles.subtitle}>
          Choose a WiFi network to connect to
        </Text>
      </View>

      {loading && (
        <View style={styles.loadingContainer}>
          <ActivityIndicator size="large" color="#007AFF" />
          <Text style={styles.loadingText}>Scanning for networks...</Text>
        </View>
      )}

      {!loading && wifiNetworks.length > 0 && (
        <>
          <FlatList
            data={wifiNetworks}
            renderItem={renderNetworkItem}
            keyExtractor={(item, index) => `${item.SSID}-${item.level || 0}-${index}`}
            style={styles.list}
          />
          <TouchableOpacity
            style={styles.scanButton}
            onPress={scanForNetworks}
            disabled={scanning}>
            <Text style={styles.scanButtonText}>
              {scanning ? 'Scanning...' : 'Refresh Scan'}
            </Text>
          </TouchableOpacity>
        </>
      )}

      {!loading && wifiNetworks.length === 0 && !scanning && (
        <View style={styles.emptyContainer}>
          <Text style={styles.emptyText}>No networks found</Text>
          <TouchableOpacity
            style={styles.scanButton}
            onPress={scanForNetworks}>
            <Text style={styles.scanButtonText}>Scan Again</Text>
          </TouchableOpacity>
        </View>
      )}

      <TouchableOpacity style={styles.cancelButton} onPress={onCancel}>
        <Text style={styles.cancelButtonText}>Cancel</Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    padding: 16,
  },
  header: {
    marginBottom: 20,
    paddingTop: 10,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
  },
  loadingContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  loadingText: {
    marginTop: 16,
    fontSize: 16,
    color: '#666',
  },
  list: {
    flex: 1,
    marginBottom: 16,
  },
  networkItem: {
    backgroundColor: '#fff',
    padding: 16,
    marginBottom: 12,
    borderRadius: 8,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  networkInfo: {
    flex: 1,
  },
  networkNameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 4,
  },
  networkName: {
    fontSize: 18,
    fontWeight: '600',
    color: '#333',
    flex: 1,
  },
  deviceBadge: {
    backgroundColor: '#007AFF',
    fontSize: 10,
    fontWeight: '600',
    paddingHorizontal: 8,
    paddingVertical: 2,
    borderRadius: 10,
    marginLeft: 8,
    overflow: 'hidden',
  },
  deviceNetworkItem: {
    borderLeftWidth: 4,
    borderLeftColor: '#007AFF',
  },
  networkDetails: {
    fontSize: 12,
    color: '#666',
  },
  selectArrow: {
    fontSize: 24,
    color: '#007AFF',
    fontWeight: 'bold',
  },
  scanButton: {
    backgroundColor: '#007AFF',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 12,
  },
  scanButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 16,
    color: '#666',
    marginBottom: 20,
  },
  cancelButton: {
    backgroundColor: '#f0f0f0',
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
  },
  cancelButtonText: {
    color: '#333',
    fontSize: 16,
    fontWeight: '600',
  },
});

export default WifiScanner;

