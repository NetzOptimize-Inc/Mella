import React, {useState} from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  KeyboardAvoidingView,
  Platform,
  ScrollView,
} from 'react-native';
import { colors } from '../theme/colors';

interface PasswordInputScreenProps {
  networkSSID: string;
  onSubmit: (password: string) => void;
  onCancel: () => void;
}

const PasswordInputScreen: React.FC<PasswordInputScreenProps> = ({
  networkSSID,
  onSubmit,
  onCancel,
}) => {
  const [password, setPassword] = useState('');

  const handleSubmit = () => {
    if (!password.trim()) {
      // Allow empty password for open networks
      onSubmit('');
    } else {
      onSubmit(password);
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
          <Text style={styles.title}>Enter WiFi Password</Text>
          <Text style={styles.subtitle}>Network: {networkSSID}</Text>
        </View>

        <View style={styles.form}>
          <View style={styles.inputGroup}>
            <Text style={styles.label}>WiFi Password</Text>
            <TextInput
              style={styles.input}
              placeholder="Enter password (leave empty for open networks)"
              placeholderTextColor={colors.placeholder}
              value={password}
              onChangeText={setPassword}
              secureTextEntry
              autoCapitalize="none"
              autoCorrect={false}
              autoFocus
            />
            <Text style={styles.hint}>
              Leave empty if this is an open network (no password required)
            </Text>
          </View>

          <View style={styles.buttonContainer}>
            <TouchableOpacity
              style={styles.submitButton}
              onPress={handleSubmit}>
              <Text style={styles.submitButtonText}>Continue</Text>
            </TouchableOpacity>

            <TouchableOpacity style={styles.cancelButton} onPress={onCancel}>
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
    backgroundColor: colors.background,
  },
  scrollContent: {
    padding: 20,
    paddingTop: 40,
  },
  header: {
    marginBottom: 24,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: colors.primary,
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 16,
    color: colors.accent,
    fontWeight: '600',
  },
  form: {
    backgroundColor: colors.surface,
    borderRadius: 12,
    padding: 20,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.08,
    shadowRadius: 4,
    elevation: 3,
  },
  inputGroup: {
    marginBottom: 24,
  },
  label: {
    fontSize: 14,
    fontWeight: '600',
    color: colors.primary,
    marginBottom: 8,
  },
  input: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: 10,
    padding: 12,
    fontSize: 16,
    backgroundColor: colors.surface,
    color: colors.primary,
    marginBottom: 8,
  },
  hint: {
    fontSize: 12,
    color: colors.secondary,
    fontStyle: 'italic',
  },
  buttonContainer: {
    marginTop: 8,
  },
  submitButton: {
    backgroundColor: colors.buttonPrimary,
    padding: 16,
    borderRadius: 10,
    alignItems: 'center',
    marginBottom: 12,
  },
  submitButtonText: {
    color: colors.buttonTextOnPrimary,
    fontSize: 17,
    fontWeight: '700',
  },
  cancelButton: {
    backgroundColor: colors.buttonSecondary,
    padding: 16,
    borderRadius: 10,
    alignItems: 'center',
  },
  cancelButtonText: {
    color: colors.buttonTextOnSecondary,
    fontSize: 16,
    fontWeight: '600',
  },
});

export default PasswordInputScreen;

