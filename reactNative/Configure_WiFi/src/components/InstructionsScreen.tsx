import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
} from 'react-native';
import { colors } from '../theme/colors';

interface InstructionsScreenProps {
  onOk: () => void;
  onCancel: () => void;
}

const InstructionsScreen: React.FC<InstructionsScreenProps> = ({
  onOk,
  onCancel,
}) => {
  return (
    <View style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <View style={styles.header}>
          <Text style={styles.title}>Setup Instructions</Text>
        </View>

        <View style={styles.content}>
          <Text style={styles.instructionTitle}>
            Step 1: Connect to SafetyBox WiFi
          </Text>
          <Text style={styles.instructionText}>
            Please connect your phone to the "SafetyBox" WiFi network manually:
          </Text>

          <View style={styles.stepsContainer}>
            <View style={styles.step}>
              <Text style={styles.stepNumber}>1</Text>
              <Text style={styles.stepText}>
                Open your phone's WiFi settings
              </Text>
            </View>

            <View style={styles.step}>
              <Text style={styles.stepNumber}>2</Text>
              <Text style={styles.stepText}>
                Look for "SafetyBox" in the WiFi list
              </Text>
            </View>

            <View style={styles.step}>
              <Text style={styles.stepNumber}>3</Text>
              <Text style={styles.stepText}>
                Tap on "SafetyBox" and connect (no password needed)
              </Text>
            </View>

            <View style={styles.step}>
              <Text style={styles.stepNumber}>4</Text>
              <Text style={styles.stepText}>
                When Android asks "No Internet, stay connected?", tap "Yes"
              </Text>
            </View>
          </View>

          <View style={styles.noteContainer}>
            <Text style={styles.noteTitle}>Note:</Text>
            <Text style={styles.noteText}>
              Make sure you are connected to SafetyBox before proceeding. The
              app will show you available WiFi networks to configure.
            </Text>
          </View>
        </View>
      </ScrollView>

      <View style={styles.buttonContainer}>
        <TouchableOpacity style={styles.okButton} onPress={onOk}>
          <Text style={styles.okButtonText}>OK, I'm Connected</Text>
        </TouchableOpacity>

        <TouchableOpacity style={styles.cancelButton} onPress={onCancel}>
          <Text style={styles.cancelButtonText}>Cancel</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: colors.background,
  },
  scrollContent: {
    padding: 20,
    paddingBottom: 100,
  },
  header: {
    marginBottom: 24,
    paddingTop: 10,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: colors.primary,
  },
  content: {
    backgroundColor: colors.surface,
    borderRadius: 12,
    padding: 20,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.08,
    shadowRadius: 4,
    elevation: 3,
  },
  instructionTitle: {
    fontSize: 20,
    fontWeight: '600',
    color: colors.primary,
    marginBottom: 12,
  },
  instructionText: {
    fontSize: 16,
    color: colors.secondary,
    marginBottom: 24,
    lineHeight: 24,
  },
  stepsContainer: {
    marginBottom: 24,
  },
  step: {
    flexDirection: 'row',
    marginBottom: 16,
    alignItems: 'flex-start',
  },
  stepNumber: {
    width: 32,
    height: 32,
    borderRadius: 16,
    backgroundColor: colors.buttonPrimary,
    color: colors.buttonTextOnPrimary,
    fontSize: 16,
    fontWeight: 'bold',
    textAlign: 'center',
    lineHeight: 32,
    marginRight: 12,
  },
  stepText: {
    flex: 1,
    fontSize: 16,
    color: colors.primary,
    lineHeight: 24,
    paddingTop: 4,
  },
  noteContainer: {
    backgroundColor: colors.warningBg,
    borderRadius: 8,
    padding: 16,
    borderLeftWidth: 4,
    borderLeftColor: colors.warningBorder,
  },
  noteTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.warningText,
    marginBottom: 8,
  },
  noteText: {
    fontSize: 14,
    color: colors.warningText,
    lineHeight: 20,
  },
  buttonContainer: {
    position: 'absolute',
    bottom: 0,
    left: 0,
    right: 0,
    backgroundColor: colors.background,
    padding: 20,
    borderTopWidth: StyleSheet.hairlineWidth,
    borderTopColor: colors.border,
  },
  okButton: {
    backgroundColor: colors.buttonPrimary,
    padding: 16,
    borderRadius: 10,
    alignItems: 'center',
    marginBottom: 12,
  },
  okButtonText: {
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

export default InstructionsScreen;

