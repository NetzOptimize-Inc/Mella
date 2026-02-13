/**
 * ESP32 WiFi Configuration App
 * Mobile application to configure ESP32 devices via WiFi AP mode
 */

import React, {useState, useEffect} from 'react';
import {
  SafeAreaView,
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ActivityIndicator,
  Modal,
  Linking,
  Platform,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import WifiScanner from './src/components/WifiScanner';
import InstructionsScreen from './src/components/InstructionsScreen';
import PasswordInputScreen from './src/components/PasswordInputScreen';
import DeviceList from './src/components/DeviceList';
import MyKeepSetupScreen from './src/components/MyKeepSetupScreen';
import MyKeepSettingsScreen from './src/components/MyKeepSettingsScreen';
import MyKeepAccountScreen from './src/components/MyKeepAccountScreen';
import MyKeepDevicesScreen from './src/components/MyKeepDevicesScreen';
import { colors } from './src/theme/colors';

const DEVICE_STORAGE_KEY = '@saved_devices';

interface WiFiNetwork {
  SSID: string;
  capabilities?: string;
  level?: number;
}

type Screen = 'home' | 'mella' | 'instructions' | 'scanner' | 'password';

type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

const App = () => {
  const [currentScreen, setCurrentScreen] = useState<Screen>('mella');
  const [myKeepTab, setMyKeepTab] = useState<MyKeepTab>('setup');
  const [selectedNetwork, setSelectedNetwork] = useState<WiFiNetwork | null>(
    null,
  );
  const [isConnecting, setIsConnecting] = useState(false);
  const [devices, setDevices] = useState<string[]>([]);

  const checkEsp32Connection = async (): Promise<boolean> => {
    try {
      // Try to reach the ESP32 at 192.168.4.1
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 2000);
      
      try {
        await fetch('http://192.168.4.1/', {
          method: 'GET',
          signal: controller.signal,
        });
        clearTimeout(timeoutId);
        // If we get any response (even 404), it means we're connected to the ESP32 network
        return true;
      } catch (error: any) {
        clearTimeout(timeoutId);
        // Network errors mean we're not connected, but other errors might mean the server responded
        if (error.name === 'AbortError' || error.message?.includes('Network request failed')) {
          return false;
        }
        // Other errors (like 404) mean we reached the server
        return true;
      }
    } catch (error) {
      return false;
    }
  };

  useEffect(() => {
    loadSavedDevices();
  }, []);


  const loadSavedDevices = async () => {
    try {
      const storedDevices = await AsyncStorage.getItem(DEVICE_STORAGE_KEY);
      if (storedDevices) {
        setDevices(JSON.parse(storedDevices));
      }
    } catch (error) {
      console.error('Error loading devices:', error);
    }
  };

  const handleAddDevice = () => {
    setCurrentScreen('instructions');
  };

  const handleInstructionsOk = () => {
    // After user clicks OK on instructions, show WiFi scanner
    setCurrentScreen('scanner');
  };

  const handleNetworkSelected = async (network: WiFiNetwork) => {
    if (!network || !network.SSID) {
      Alert.alert('Error', 'Invalid network selected');
      return;
    }

    // Store the selected network and prompt for password
    setSelectedNetwork(network);
    setCurrentScreen('password');
  };

  const handlePasswordEntered = async (password: string) => {
    if (!selectedNetwork || !selectedNetwork.SSID) {
      Alert.alert('Error', 'No network selected');
      return;
    }

    setIsConnecting(true);

    try {
      // Verify we're connected to SafetyBox WiFi and can reach ESP32
      const canReachEsp32 = await checkEsp32Connection();
      
      if (!canReachEsp32) {
        Alert.alert(
          'Not Connected to SafetyBox',
          'Please make sure you are connected to the SafetyBox WiFi network.\n\nTap "Open WiFi Settings" to connect manually.',
          [
            {
              text: 'Open WiFi Settings',
              onPress: () => {
                if (Platform.OS === 'android') {
                  Linking.openURL('android.settings.WIFI_SETTINGS').catch(() => {
                    Linking.openSettings();
                  });
                } else {
                  Linking.openSettings();
                }
                setIsConnecting(false);
              },
            },
            {
              text: 'Cancel',
              onPress: () => {
                setIsConnecting(false);
              },
            },
          ],
        );
        return;
      }

      // Send credentials to ESP32
      await sendCredentialsToESP32(selectedNetwork.SSID, password);
    } catch (error: any) {
      console.error('Error sending credentials:', error);
      setIsConnecting(false);
      Alert.alert(
        'Error',
        `Failed to send credentials: ${error.message}`,
        [
          {text: 'Retry', onPress: () => handlePasswordEntered(password)},
          {text: 'Cancel', onPress: () => setIsConnecting(false)},
        ],
      );
    }
  };

  const sendCredentialsToESP32 = async (ssid: string, password: string) => {
    try {
      console.log('Sending credentials to ESP32...');
      console.log('SSID:', ssid);
      console.log('Password:', password ? '***' : '(empty)');

      // Prepare form data
      const formDataString = `ssid=${encodeURIComponent(ssid.trim())}&pass=${encodeURIComponent(password.trim())}`;
      
      // Create AbortController for timeout
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 20000);

      // Send POST request to ESP32
      const response = await fetch('http://192.168.4.1/save', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded',
          'Accept': '*/*',
          'Connection': 'keep-alive',
        },
        body: formDataString,
        signal: controller.signal,
      } as any);

      clearTimeout(timeoutId);

      const responseText = await response.text();
      console.log('Response status:', response.status);
      console.log('Response text:', responseText);

      if (response.ok || response.status === 200) {
        setIsConnecting(false);
        // Save device name locally using the selected WiFi network SSID
        await handleConfigSuccess(ssid);
      } else {
        throw new Error(`Server responded with status: ${response.status}`);
      }
    } catch (error: any) {
      console.error('Send credentials error:', error);
      throw error;
    }
  };


  const handleConfigSuccess = async (deviceName: string) => {
    try {
      // Save device to local storage
      const updatedDevices = [...devices, deviceName];
      await AsyncStorage.setItem(
        DEVICE_STORAGE_KEY,
        JSON.stringify(updatedDevices),
      );
      setDevices(updatedDevices);

      Alert.alert(
        'Success!',
        `Device "${deviceName}" has been configured and saved.`,
        [
          {
            text: 'OK',
            onPress: () => {
              resetToMella();
            },
          },
        ],
      );
    } catch (error) {
      console.error('Error saving device:', error);
      Alert.alert(
        'Warning',
        'Device configured but failed to save device name.',
      );
      resetToMella();
    }
  };

  const resetToHome = () => {
    setCurrentScreen('home');
    setSelectedNetwork(null);
    setIsConnecting(false);
    loadSavedDevices();
  };

  /** Return to Mella main UI (Setup tab) after cancel or success in add-device flow. */
  const resetToMella = () => {
    setCurrentScreen('mella');
    setSelectedNetwork(null);
    setIsConnecting(false);
    loadSavedDevices();
  };

  const renderHomeScreen = () => (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.appTitle}>ESP32 WiFi Config</Text>
        <Text style={styles.appSubtitle}>
          Configure your ESP32 devices via WiFi
        </Text>
      </View>

      <View style={styles.content}>
        <TouchableOpacity
          style={styles.addDeviceButton}
          onPress={handleAddDevice}
          activeOpacity={0.8}>
          <Text style={styles.addDeviceButtonText}>+ Add Device</Text>
        </TouchableOpacity>

        <View style={styles.deviceListContainer}>
          <Text style={styles.deviceListTitle}>Configured Devices</Text>
          <DeviceList
            devices={devices}
            onDeviceRemoved={updatedDevices => {
              setDevices(updatedDevices);
            }}
          />
        </View>
      </View>
    </View>
  );

  return (
    <SafeAreaView style={styles.safeArea}>
      {currentScreen === 'home' && renderHomeScreen()}

      {currentScreen === 'mella' && (
        <View style={styles.mellaTabsContainer}>
          <View
            style={[
              styles.mellaTabPage,
              myKeepTab !== 'setup' && styles.mellaTabPageHidden,
            ]}>
            <MyKeepSetupScreen
              onNext={handleAddDevice}
              onTab={setMyKeepTab}
              activeTab={myKeepTab}
            />
          </View>
          <View
            style={[
              styles.mellaTabPage,
              myKeepTab !== 'devices' && styles.mellaTabPageHidden,
            ]}>
            <MyKeepDevicesScreen onTab={setMyKeepTab} />
          </View>
          <View
            style={[
              styles.mellaTabPage,
              myKeepTab !== 'settings' && styles.mellaTabPageHidden,
            ]}>
            <MyKeepSettingsScreen onTab={setMyKeepTab} />
          </View>
          <View
            style={[
              styles.mellaTabPage,
              myKeepTab !== 'account' && styles.mellaTabPageHidden,
            ]}>
            <MyKeepAccountScreen onTab={setMyKeepTab} />
          </View>
        </View>
      )}

      {currentScreen === 'instructions' && (
        <InstructionsScreen
          onOk={handleInstructionsOk}
          onCancel={resetToMella}
        />
      )}

      {currentScreen === 'scanner' && (
        <WifiScanner
          onNetworkSelected={handleNetworkSelected}
          onCancel={resetToMella}
        />
      )}

      {currentScreen === 'password' && selectedNetwork && (
        <PasswordInputScreen
          networkSSID={selectedNetwork.SSID}
          onSubmit={handlePasswordEntered}
          onCancel={resetToMella}
        />
      )}

      <Modal transparent visible={isConnecting} animationType="fade">
        <View style={styles.modalOverlay}>
          <View style={styles.modalContent}>
            <ActivityIndicator size="large" color={colors.accent} />
            <Text style={styles.modalText}>Sending credentials...</Text>
            <Text style={styles.modalSubtext}>
              Please wait while we configure the device
            </Text>
          </View>
        </View>
      </Modal>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  safeArea: {
    flex: 1,
    backgroundColor: colors.surface,
  },
  mellaTabsContainer: {
    flex: 1,
  },
  mellaTabPage: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    bottom: 0,
    zIndex: 1,
  },
  mellaTabPageHidden: {
    opacity: 0,
    pointerEvents: 'none',
    zIndex: 0,
  },
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  header: {
    backgroundColor: colors.surface,
    padding: 20,
    paddingTop: 10,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: colors.border,
  },
  appTitle: {
    fontSize: 28,
    fontWeight: 'bold',
    color: colors.primary,
    marginBottom: 4,
  },
  appSubtitle: {
    fontSize: 14,
    color: colors.secondary,
  },
  content: {
    flex: 1,
    padding: 20,
  },
  addDeviceButton: {
    backgroundColor: colors.buttonPrimary,
    padding: 18,
    borderRadius: 12,
    alignItems: 'center',
    marginBottom: 24,
    shadowColor: colors.accent,
    shadowOffset: {width: 0, height: 4},
    shadowOpacity: 0.25,
    shadowRadius: 8,
    elevation: 5,
  },
  addDeviceButtonText: {
    color: colors.buttonTextOnPrimary,
    fontSize: 18,
    fontWeight: '700',
  },
  deviceListContainer: {
    flex: 1,
  },
  deviceListTitle: {
    fontSize: 20,
    fontWeight: '600',
    color: colors.primary,
    marginBottom: 16,
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: colors.overlay,
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContent: {
    backgroundColor: colors.surface,
    borderRadius: 12,
    padding: 30,
    alignItems: 'center',
    minWidth: 250,
  },
  modalText: {
    marginTop: 16,
    fontSize: 16,
    fontWeight: '600',
    color: colors.primary,
  },
  modalSubtext: {
    marginTop: 8,
    fontSize: 14,
    color: colors.secondary,
    textAlign: 'center',
  },
});

export default App;
