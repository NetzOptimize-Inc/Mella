/**
 * MyKeep Settings screen
 * Sections: Notifications, Device Settings, App Settings, Data & Support.
 */

import React, {useState} from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Switch,
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';
import MyKeepLogo from './MyKeepLogo';

const ACCENT_ORANGE = '#FF6600';
const PRIMARY_GREY = '#333333';
const SECONDARY_GREY = '#666666';
const BG_WHITE = '#FFFFFF';
const BG_OFF_WHITE = '#F9F9F9';
const CARD_BG = '#F0F0F0';
const INACTIVE_STEP = '#DDDDDD';
const TAB_ACTIVE_BG = '#FFF0E6';

export type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

interface MyKeepSettingsScreenProps {
  onTab?: (tab: MyKeepTab) => void;
}

const MyKeepSettingsScreen: React.FC<MyKeepSettingsScreenProps> = ({
  onTab,
}) => {
  const [packageAlerts, setPackageAlerts] = useState(true);
  const [darkMode, setDarkMode] = useState(false);

  const renderSettingRow = (
    icon: React.ReactNode,
    title: string,
    subtitle: string,
    right?: React.ReactNode,
  ) => (
    <View style={styles.card}>
      <View style={styles.cardIcon}>{icon}</View>
      <View style={styles.cardText}>
        <Text style={styles.cardTitle}>{title}</Text>
        <Text style={styles.cardSubtitle}>{subtitle}</Text>
      </View>
      {right != null ? <View style={styles.cardRight}>{right}</View> : null}
    </View>
  );

  const renderTappableRow = (
    icon: React.ReactNode,
    title: string,
    subtitle: string,
    onPress: () => void,
  ) => (
    <TouchableOpacity
      style={styles.card}
      onPress={onPress}
      activeOpacity={0.7}>
      <View style={styles.cardIcon}>{icon}</View>
      <View style={styles.cardText}>
        <Text style={styles.cardTitle}>{title}</Text>
        <Text style={styles.cardSubtitle}>{subtitle}</Text>
      </View>
      <MaterialCommunityIcons name="chevron-right" size={24} color={SECONDARY_GREY} />
    </TouchableOpacity>
  );

  const Section = ({
    title,
    children,
  }: {
    title: string;
    children: React.ReactNode;
  }) => (
    <View style={styles.section}>
      <Text style={styles.sectionTitle}>{title}</Text>
      {children}
    </View>
  );

  return (
    <View style={styles.outer}>
      {/* Header - white */}
      <View style={styles.header}>
        <View style={styles.logoIcon}>
          <MyKeepLogo size={28} />
        </View>
        <Text style={styles.headerTitle}>The Mella</Text>
      </View>

      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}>
        {/* <Section title="NOTIFICATIONS">
          {renderSettingRow(
            <MaterialCommunityIcons name="bell-outline" size={24} color={ACCENT_ORANGE} />,
            'Package Delivery Alerts',
            'Get notified when packages arrive',
            <Switch
              value={packageAlerts}
              onValueChange={setPackageAlerts}
              trackColor={{false: INACTIVE_STEP, true: ACCENT_ORANGE}}
              thumbColor={BG_WHITE}
            />,
          )}
        </Section> */}

        <Section title="DEVICE SETTINGS">
          {renderSettingRow(
            <MaterialCommunityIcons name="wifi" size={24} color={ACCENT_ORANGE} />,
            'WiFi Password',
            'Change Mella box WiFi password',
          )}
          {renderSettingRow(
            <MaterialCommunityIcons name="lock-outline" size={24} color={ACCENT_ORANGE} />,
            'Security',
            'View and manage security settings',
          )}
        </Section>

        <Section title="APP SETTINGS">
          {renderSettingRow(
            <MaterialCommunityIcons name="weather-night" size={24} color={ACCENT_ORANGE} />,
            'Dark Mode',
            'Enable dark theme for the app',
            <Switch
              value={darkMode}
              onValueChange={setDarkMode}
              trackColor={{false: INACTIVE_STEP, true: ACCENT_ORANGE}}
              thumbColor={BG_WHITE}
            />,
          )}
        </Section>

        <Section title="DATA & SUPPORT">
          {renderSettingRow(
            <MaterialCommunityIcons name="download" size={24} color={ACCENT_ORANGE} />,
            'Export Data',
            'Download all your activity logs',
          )}
        </Section>

        <View style={styles.footer}>
          <Text style={styles.footerVersion}>The Mella Mobile App v1.0.0</Text>
          <Text style={styles.footerCopyright}>© 2024 Mella. All rights reserved.</Text>
        </View>
      </ScrollView>

      {/* Bottom navigation - Settings active */}
      <View style={styles.bottomNav}>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('setup')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="lightning-bolt" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Setup</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('devices')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="calendar-clock" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Scheduler</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, styles.navItemActive]}
          onPress={() => onTab?.('settings')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="cog" size={24} color={ACCENT_ORANGE} style={styles.navIcon} />
          <Text style={[styles.navLabel, styles.navLabelActive]}>Settings</Text>
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
    backgroundColor: BG_OFF_WHITE,
  },
  scrollContent: {
    paddingHorizontal: 20,
    paddingTop: 24,
    paddingBottom: 24,
  },
  section: {
    marginBottom: 28,
  },
  sectionTitle: {
    fontSize: 13,
    fontWeight: '700',
    color: ACCENT_ORANGE,
    letterSpacing: 0.5,
    marginBottom: 12,
    paddingHorizontal: 4,
  },
  card: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: CARD_BG,
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
  },
  cardIcon: {
    width: 40,
    height: 40,
    borderRadius: 20,
    backgroundColor: 'transparent',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 14,
  },
  cardText: {
    flex: 1,
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: PRIMARY_GREY,
    marginBottom: 2,
  },
  cardSubtitle: {
    fontSize: 13,
    color: SECONDARY_GREY,
  },
  cardRight: {
    marginLeft: 12,
  },
  footer: {
    alignItems: 'center',
    marginTop: 16,
    marginBottom: 8,
  },
  footerVersion: {
    fontSize: 14,
    color: PRIMARY_GREY,
    marginBottom: 4,
  },
  footerCopyright: {
    fontSize: 13,
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

export default MyKeepSettingsScreen;
