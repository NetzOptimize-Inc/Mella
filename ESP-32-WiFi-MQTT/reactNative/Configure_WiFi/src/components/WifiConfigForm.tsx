import React, {useState} from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  Alert,
  ActivityIndicator,
  ScrollView,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';

interface WifiConfigFormProps {
  networkSSID: string;
  onSuccess: (deviceName: string) => void;
  onCancel: () => void;
}

const WifiConfigForm: React.FC<WifiConfigFormProps> = ({
  networkSSID,
  onSuccess,
  onCancel,
}) => {
  const [ssid, setSsid] = useState('Airtel_NetzOptimize');
  const [password, setPassword] = useState('Airtel@123');
  const [loading, setLoading] = useState(false);

  const handleSubmit = async () => {
    if (!ssid.trim()) {
      Alert.alert('Error', 'Please enter a WiFi network name (SSID)');
      return;
    }

    if (!password.trim()) {
      Alert.alert('Error', 'Please enter a WiFi password');
      return;
    }

    setLoading(true);

    try {
      // First, verify we can reach the ESP32 before sending data
      console.log('Verifying ESP32 connection...');
      const testController = new AbortController();
      const testTimeoutId = setTimeout(() => testController.abort(), 3000);
      const testResponse = await fetch('http://192.168.4.1/', {
        method: 'GET',
        signal: testController.signal,
      } as any).catch(() => null);
      clearTimeout(testTimeoutId);

      if (!testResponse) {
        throw new Error('Cannot reach ESP32. Please ensure you are connected to ' + networkSSID);
      }

      console.log('ESP32 is reachable, sending credentials...');

      // Prepare form data as URL-encoded string (matching ESP32 server.arg() format)
      const formDataString = `ssid=${encodeURIComponent(ssid.trim())}&pass=${encodeURIComponent(password.trim())}`;
      
      console.log('Sending POST request to http://192.168.4.1/save');
      console.log('Form data:', formDataString);

      // Create AbortController for timeout
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 20000); // Increased timeout

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
        redirect: 'follow',
      } as any);

      clearTimeout(timeoutId);

      console.log('Response status:', response.status);
      console.log('Response ok:', response.ok);

      // Read response text for debugging
      const responseText = await response.text();
      console.log('Response text:', responseText);

      if (response.ok || response.status === 200) {
        Alert.alert(
          'Success',
          'WiFi credentials saved successfully! The device will reboot.',
          [
            {
              text: 'OK',
              onPress: () => {
                // Save device name locally
                onSuccess(networkSSID);
              },
            },
          ],
        );
      } else {
        throw new Error(`Server responded with status: ${response.status}`);
      }
    } catch (error: any) {
      console.error('Save error:', error);
      console.error('Error details:', JSON.stringify(error, null, 2));
      
      let errorMessage = error.message || 'Unknown error';
      
      if (error.name === 'AbortError' || error.name === 'TimeoutError') {
        errorMessage = 'Request timeout. Please check your connection to the ESP32.';
      } else if (error.message?.includes('Network request failed') || error.message?.includes('Failed to fetch')) {
        errorMessage = 'Network request failed. Please ensure:\n1. You are connected to ' + networkSSID + '\n2. ESP32 is accessible at 192.168.4.1\n3. Try disconnecting and reconnecting to the network';
      } else if (error.message?.includes('Cannot reach ESP32')) {
        errorMessage = error.message;
      }
      
      Alert.alert(
        'Connection Error',
        `Failed to save WiFi credentials: ${errorMessage}\n\nPlease ensure:\n1. You are connected to the ${networkSSID} network\n2. The ESP32 is accessible at 192.168.4.1\n3. Try disconnecting and reconnecting to the network\n4. Try again in a few seconds`,
        [
          {text: 'Retry', onPress: () => setLoading(false)},
          {text: 'Cancel', onPress: onCancel},
        ],
      );
    } finally {
      setLoading(false);
    }
  };

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}>
      <ScrollView
        contentContainerStyle={styles.scrollContent}
        keyboardShouldPersistTaps="handled">
        <View style={styles.header}>
          <Text style={styles.title}>Configure WiFi</Text>
          <Text style={styles.subtitle}>Connected to: {networkSSID}</Text>
          <Text style={styles.description}>
            Enter the WiFi network credentials you want to save on your ESP32
            device.
          </Text>
        </View>

        <View style={styles.form}>
          <View style={styles.inputGroup}>
            <Text style={styles.label}>WiFi Network Name (SSID)</Text>
            <TextInput
              style={styles.input}
              placeholder="Enter WiFi SSID"
              placeholderTextColor="#999"
              value={ssid}
              onChangeText={setSsid}
              autoCapitalize="none"
              autoCorrect={false}
              editable={!loading}
            />
          </View>

          <View style={styles.inputGroup}>
            <Text style={styles.label}>WiFi Password</Text>
            <TextInput
              style={styles.input}
              placeholder="Enter WiFi Password"
              placeholderTextColor="#999"
              value={password}
              onChangeText={setPassword}
              secureTextEntry
              autoCapitalize="none"
              autoCorrect={false}
              editable={!loading}
            />
          </View>

          {loading && (
            <View style={styles.loadingContainer}>
              <ActivityIndicator size="large" color="#007AFF" />
              <Text style={styles.loadingText}>Saving credentials...</Text>
            </View>
          )}

          <View style={styles.buttonContainer}>
            <TouchableOpacity
              style={[styles.button, styles.continueButton, loading && styles.buttonDisabled]}
              onPress={handleSubmit}
              disabled={loading}>
              <Text style={styles.continueButtonText}>Continue</Text>
            </TouchableOpacity>

            <TouchableOpacity
              style={[styles.button, styles.cancelButton]}
              onPress={onCancel}
              disabled={loading}>
              <Text style={styles.cancelButtonText}>Cancel</Text>
            </TouchableOpacity>
          </View>
        </View>
      </ScrollView>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  scrollContent: {
    padding: 16,
    paddingTop: 20,
  },
  header: {
    marginBottom: 24,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 16,
    color: '#007AFF',
    fontWeight: '600',
    marginBottom: 8,
  },
  description: {
    fontSize: 14,
    color: '#666',
    lineHeight: 20,
  },
  form: {
    backgroundColor: '#fff',
    borderRadius: 12,
    padding: 20,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  inputGroup: {
    marginBottom: 20,
  },
  label: {
    fontSize: 14,
    fontWeight: '600',
    color: '#333',
    marginBottom: 8,
  },
  input: {
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 8,
    padding: 12,
    fontSize: 16,
    backgroundColor: '#fff',
    color: '#333',
  },
  loadingContainer: {
    alignItems: 'center',
    marginVertical: 20,
  },
  loadingText: {
    marginTop: 12,
    fontSize: 14,
    color: '#666',
  },
  buttonContainer: {
    marginTop: 8,
  },
  button: {
    padding: 16,
    borderRadius: 8,
    alignItems: 'center',
    marginBottom: 12,
  },
  continueButton: {
    backgroundColor: '#007AFF',
  },
  buttonDisabled: {
    backgroundColor: '#ccc',
  },
  continueButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  cancelButton: {
    backgroundColor: '#f0f0f0',
  },
  cancelButtonText: {
    color: '#333',
    fontSize: 16,
    fontWeight: '600',
  },
});

export default WifiConfigForm;

