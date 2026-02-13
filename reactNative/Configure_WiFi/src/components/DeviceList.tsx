import React from 'react';
import {
  View,
  Text,
  FlatList,
  StyleSheet,
  TouchableOpacity,
  Alert,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { colors } from '../theme/colors';

const DEVICE_STORAGE_KEY = '@saved_devices';

interface DeviceListProps {
  devices: string[];
  onDeviceRemoved: (updatedDevices: string[]) => void;
}

const DeviceList: React.FC<DeviceListProps> = ({devices, onDeviceRemoved}) => {
  const removeDevice = async (deviceName: string) => {
    Alert.alert(
      'Remove Device',
      `Are you sure you want to remove "${deviceName}"?`,
      [
        {text: 'Cancel', style: 'cancel'},
        {
          text: 'Remove',
          style: 'destructive',
          onPress: async () => {
            try {
              const updatedDevices = devices.filter(d => d !== deviceName);
              await AsyncStorage.setItem(
                DEVICE_STORAGE_KEY,
                JSON.stringify(updatedDevices),
              );
              // Notify parent component
              if (onDeviceRemoved) {
                onDeviceRemoved(updatedDevices);
              }
            } catch (error) {
              console.error('Error removing device:', error);
              Alert.alert('Error', 'Failed to remove device');
            }
          },
        },
      ],
    );
  };

  const renderDeviceItem = ({item}: {item: string}) => (
    <View style={styles.deviceItem}>
      <View style={styles.deviceInfo}>
        <Text style={styles.deviceName}>{item}</Text>
        <Text style={styles.deviceStatus}>Configured</Text>
      </View>
      <TouchableOpacity
        style={styles.removeButton}
        onPress={() => removeDevice(item)}>
        <Text style={styles.removeButtonText}>Remove</Text>
      </TouchableOpacity>
    </View>
  );

  return (
    <View style={styles.container}>
      {devices.length === 0 ? (
        <View style={styles.emptyContainer}>
          <Text style={styles.emptyText}>
            No devices saved yet. Add a device to get started.
          </Text>
        </View>
      ) : (
        <FlatList
          data={devices}
          renderItem={renderDeviceItem}
          keyExtractor={(item, index) => `${item}-${index}`}
          style={styles.list}
        />
      )}
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  list: {
    flex: 1,
  },
  deviceItem: {
    backgroundColor: colors.surface,
    padding: 16,
    marginBottom: 12,
    borderRadius: 12,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.08,
    shadowRadius: 4,
    elevation: 3,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    fontSize: 18,
    fontWeight: '600',
    color: colors.primary,
    marginBottom: 4,
  },
  deviceStatus: {
    fontSize: 12,
    color: colors.success,
  },
  removeButton: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 8,
    backgroundColor: colors.error,
  },
  removeButtonText: {
    color: colors.buttonTextOnPrimary,
    fontSize: 14,
    fontWeight: '600',
  },
  emptyContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    paddingVertical: 40,
  },
  emptyText: {
    fontSize: 16,
    color: colors.secondary,
    textAlign: 'center',
  },
});

export default DeviceList;

