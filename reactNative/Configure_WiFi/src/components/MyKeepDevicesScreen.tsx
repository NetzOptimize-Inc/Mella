/**
 * MyKeep Devices screen
 * Header with package icon, list of device cards (name, Online status, state, last activity).
 */

import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';
import MyKeepLogo from './MyKeepLogo';

const ACCENT_ORANGE = '#FF6600';
const PRIMARY_GREY = '#333333';
const SECONDARY_GREY = '#666666';
const BG_WHITE = '#FFFFFF';
const BG_OFF_WHITE = '#F9F9F9';
const INACTIVE_STEP = '#DDDDDD';
const TAB_ACTIVE_BG = '#FFF0E6';
const ONLINE_GREEN = '#4CAF50';

export type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

export interface DeviceItem {
  id: string;
  name: string;
  online: boolean;
  isOpen: boolean;
  lastActivity: string;
}

interface MyKeepDevicesScreenProps {
  onTab?: (tab: MyKeepTab) => void;
  devices?: DeviceItem[];
}

const DEFAULT_DEVICES: DeviceItem[] = [
  { id: '1', name: 'Front Delivery Box', online: true, isOpen: false, lastActivity: '2 hours ago' },
  { id: '2', name: 'Back Entrance', online: true, isOpen: true, lastActivity: 'Just now' },
];

const MyKeepDevicesScreen: React.FC<MyKeepDevicesScreenProps> = ({
  onTab,
  devices = DEFAULT_DEVICES,
}) => {
  const renderDeviceCard = (device: DeviceItem) => (
    <TouchableOpacity
      key={device.id}
      style={styles.deviceCard}
      activeOpacity={0.8}>
      <View style={styles.deviceCardContent}>
        <View style={styles.deviceCardHeader}>
          <Text style={styles.deviceName}>{device.name}</Text>
          <View style={styles.statusRow}>
            <View style={[styles.statusDot, device.online && styles.statusDotOnline]} />
            <Text style={[styles.statusText, device.online && styles.statusTextOnline]}>
              {device.online ? 'Online' : 'Offline'}
            </Text>
          </View>
        </View>
        <Text style={styles.deviceState}>
          Currently {device.isOpen ? 'Open' : 'Closed'}
        </Text>
        <Text style={styles.deviceActivity}>
          Last activity {device.lastActivity}
        </Text>
      </View>
      <View style={styles.chevronWrap}>
        <MaterialCommunityIcons
          name="chevron-down"
          size={24}
          color={SECONDARY_GREY}
        />
      </View>
    </TouchableOpacity>
  );

  return (
    <View style={styles.outer}>
      <View style={styles.header}>
        <View style={styles.logoIcon}>
          <MyKeepLogo size={28} />
        </View>
        <Text style={styles.headerTitle}>MyKeep</Text>
      </View>

      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}>
        {devices.length === 0 ? (
          <View style={styles.emptyWrap}>
            <Text style={styles.emptyText}>No devices yet.</Text>
            <Text style={styles.emptySubtext}>Add a device from Setup to get started.</Text>
          </View>
        ) : (
          devices.map(renderDeviceCard)
        )}
      </ScrollView>

      <View style={styles.bottomNav}>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('setup')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="lightning-bolt" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Setup</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, styles.navItemActive]}
          onPress={() => onTab?.('devices')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="cube-outline" size={24} color={ACCENT_ORANGE} style={styles.navIcon} />
          <Text style={[styles.navLabel, styles.navLabelActive]}>Devices</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('settings')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="cog" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Settings</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('account')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="account" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Account</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  outer: {
    flex: 1,
    backgroundColor: BG_OFF_WHITE,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: BG_WHITE,
    paddingHorizontal: 20,
    paddingVertical: 14,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: INACTIVE_STEP,
  },
  logoIcon: {
    marginRight: 10,
  },
  headerTitle: {
    fontSize: 20,
    fontWeight: '700',
    color: PRIMARY_GREY,
    letterSpacing: 0.3,
  },
  scroll: {
    flex: 1,
  },
  scrollContent: {
    paddingHorizontal: 20,
    paddingTop: 24,
    paddingBottom: 24,
  },
  deviceCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: BG_WHITE,
    borderRadius: 12,
    padding: 16,
    marginBottom: 12,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.08,
    shadowRadius: 4,
    elevation: 3,
  },
  deviceCardContent: {
    flex: 1,
  },
  deviceCardHeader: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 6,
  },
  deviceName: {
    fontSize: 17,
    fontWeight: '700',
    color: PRIMARY_GREY,
    flex: 1,
  },
  statusRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginLeft: 8,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: SECONDARY_GREY,
    marginRight: 6,
  },
  statusDotOnline: {
    backgroundColor: ONLINE_GREEN,
  },
  statusText: {
    fontSize: 13,
    color: SECONDARY_GREY,
    fontWeight: '500',
  },
  statusTextOnline: {
    color: ONLINE_GREEN,
  },
  deviceState: {
    fontSize: 14,
    color: SECONDARY_GREY,
    marginBottom: 2,
  },
  deviceActivity: {
    fontSize: 13,
    color: SECONDARY_GREY,
  },
  chevronWrap: {
    marginLeft: 12,
  },
  emptyWrap: {
    paddingVertical: 48,
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 17,
    fontWeight: '600',
    color: PRIMARY_GREY,
    marginBottom: 8,
  },
  emptySubtext: {
    fontSize: 14,
    color: SECONDARY_GREY,
  },
  bottomNav: {
    flexDirection: 'row',
    backgroundColor: BG_WHITE,
    borderTopWidth: StyleSheet.hairlineWidth,
    borderTopColor: INACTIVE_STEP,
    paddingVertical: 10,
    paddingBottom: 24,
    paddingHorizontal: 8,
  },
  navItem: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingVertical: 8,
    borderRadius: 8,
  },
  navItemActive: {
    backgroundColor: TAB_ACTIVE_BG,
  },
  navIcon: {
    marginBottom: 4,
  },
  navLabel: {
    fontSize: 12,
    color: PRIMARY_GREY,
    fontWeight: '500',
  },
  navLabelActive: {
    color: ACCENT_ORANGE,
    fontWeight: '600',
  },
});

export default MyKeepDevicesScreen;
