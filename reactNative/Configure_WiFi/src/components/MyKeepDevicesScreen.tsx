/**
 * MyKeep Scheduler tab (formerly Devices).
 * Shows the Weekly Scheduler UI with bottom nav.
 */

import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';
import SchedulerScreen from './SchedulerScreen';

const ACCENT_ORANGE = '#FF6600';
const PRIMARY_GREY = '#333333';
const BG_WHITE = '#FFFFFF';
const INACTIVE_STEP = '#DDDDDD';
const TAB_ACTIVE_BG = '#FFF0E6';

export type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

interface MyKeepDevicesScreenProps {
  onTab?: (tab: MyKeepTab) => void;
}

const MyKeepDevicesScreen: React.FC<MyKeepDevicesScreenProps> = ({ onTab }) => {
  return (
    <View style={styles.outer}>
      <View style={styles.schedulerWrap}>
        <SchedulerScreen
          showBackButton={false}
          onBack={() => {}}
          onSchedule={(deviceId, schedule) => {
            // Optional: persist or send to device
          }}
        />
      </View>

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
          <MaterialCommunityIcons name="calendar-clock" size={24} color={ACCENT_ORANGE} style={styles.navIcon} />
          <Text style={[styles.navLabel, styles.navLabelActive]}>Scheduler</Text>
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
    backgroundColor: BG_WHITE,
  },
  schedulerWrap: {
    flex: 1,
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
