/**
 * MyKeep Account screen
 * Profile card, Profile Information (Email, Phone, Location), Account (Privacy Policy).
 */

import React from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Linking,
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';
import MyKeepLogo from './MyKeepLogo';

const ACCENT_ORANGE = '#FF6600';
const PRIMARY_GREY = '#333333';
const SECONDARY_GREY = '#666666';
const BG_WHITE = '#FFFFFF';
const BG_OFF_WHITE = '#F9F9F9';
const CARD_BG = '#FFFFFF';
const INACTIVE_STEP = '#DDDDDD';
const TAB_ACTIVE_BG = '#FFF0E6';
const PROFILE_CARD_BG = '#FFF0E6';
const LOGOUT_RED = '#D32F2F';
const LOGOUT_BG = '#FFEBEE';

export type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

interface MyKeepAccountScreenProps {
  onTab?: (tab: MyKeepTab) => void;
}

const MyKeepAccountScreen: React.FC<MyKeepAccountScreenProps> = ({onTab}) => {
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

  const renderProfileRow = (
    icon: React.ReactNode,
    label: string,
    value: string,
  ) => (
    <View style={styles.profileRow}>
      <View style={styles.profileRowIcon}>{icon}</View>
      <View style={styles.profileRowText}>
        <Text style={styles.profileRowLabel}>{label}</Text>
        <Text style={styles.profileRowValue}>{value}</Text>
      </View>
    </View>
  );

  return (
    <View style={styles.outer}>
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
        {/* Profile card */}
        <View style={styles.profileCard}>
          <View style={styles.profileAvatar}>
            <MaterialCommunityIcons name="account" size={32} color={BG_WHITE} />
          </View>
          <View style={styles.profileInfo}>
            <Text style={styles.profileName}>John Doe</Text>
            <Text style={styles.profileSince}>Member since January 2024</Text>
          </View>
        </View>

        <Section title="PROFILE INFORMATION">
          {renderProfileRow(
            <MaterialCommunityIcons name="email-outline" size={24} color={ACCENT_ORANGE} />,
            'Email',
            'john@example.com',
          )}
          {renderProfileRow(
            <MaterialCommunityIcons name="phone-outline" size={24} color={ACCENT_ORANGE} />,
            'Phone',
            '+1 (555) 123-4567',
          )}
          {renderProfileRow(
            <MaterialCommunityIcons name="map-marker-outline" size={24} color={ACCENT_ORANGE} />,
            'Location',
            'New York, USA',
          )}
        </Section>

        <Section title="ACCOUNT">
          <TouchableOpacity
            style={styles.accountRow}
            activeOpacity={0.7}>
            <View style={styles.accountRowIcon}>
              <MaterialCommunityIcons name="shield-account-outline" size={24} color={ACCENT_ORANGE} />
            </View>
            <Text style={styles.accountRowTitle}>Privacy Policy</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.accountRow}
            activeOpacity={0.7}>
            <View style={styles.accountRowIcon}>
              <MaterialCommunityIcons name="help-circle-outline" size={24} color={ACCENT_ORANGE} />
            </View>
            <Text style={styles.accountRowTitle}>Help & Support</Text>
          </TouchableOpacity>
          <TouchableOpacity
            style={styles.logoutButton}
            activeOpacity={0.7}>
            <View style={styles.logoutIconWrap}>
              <MaterialCommunityIcons name="logout" size={24} color={LOGOUT_RED} />
            </View>
            <Text style={styles.logoutText}>Logout</Text>
          </TouchableOpacity>
        </Section>

        <View style={styles.accountFooter}>
          <Text style={styles.accountFooterTitle}>The Mella Account Settings</Text>
          <Text style={styles.accountFooterLine}>
            For account assistance, contact{' '}
            <Text
              style={styles.accountFooterLink}
              
              onPress={() => Linking.openURL('mailto:support@themella.com')}>
              support@themella.com
            </Text>
          </Text>
        </View>
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
          style={styles.navItem}
          onPress={() => onTab?.('devices')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="calendar-clock" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Scheduler</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={styles.navItem}
          onPress={() => onTab?.('settings')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="cog" size={24} color={PRIMARY_GREY} style={styles.navIcon} />
          <Text style={styles.navLabel}>Settings</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, styles.navItemActive]}
          onPress={() => onTab?.('account')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons name="account" size={24} color={ACCENT_ORANGE} style={styles.navIcon} />
          <Text style={[styles.navLabel, styles.navLabelActive]}>Account</Text>
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
  profileCard: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: PROFILE_CARD_BG,
    borderRadius: 12,
    padding: 16,
    marginBottom: 28,
  },
  profileAvatar: {
    width: 48,
    height: 48,
    borderRadius: 24,
    backgroundColor: ACCENT_ORANGE,
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 14,
  },
  profileInfo: {
    flex: 1,
  },
  profileName: {
    fontSize: 20,
    fontWeight: '700',
    color: PRIMARY_GREY,
    marginBottom: 4,
  },
  profileSince: {
    fontSize: 14,
    color: SECONDARY_GREY,
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
  profileRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: CARD_BG,
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
  },
  profileRowIcon: {
    width: 40,
    height: 40,
    borderRadius: 20,
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 14,
  },
  profileRowText: {
    flex: 1,
  },
  profileRowLabel: {
    fontSize: 13,
    color: SECONDARY_GREY,
    marginBottom: 2,
  },
  profileRowValue: {
    fontSize: 16,
    fontWeight: '600',
    color: PRIMARY_GREY,
  },
  accountRow: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: CARD_BG,
    borderRadius: 12,
    padding: 16,
    marginBottom: 10,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: INACTIVE_STEP,
  },
  accountRowIcon: {
    width: 40,
    height: 40,
    borderRadius: 20,
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 14,
  },
  accountRowTitle: {
    fontSize: 16,
    fontWeight: '700',
    color: PRIMARY_GREY,
  },
  logoutButton: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: LOGOUT_BG,
    borderRadius: 12,
    padding: 16,
    marginTop: 8,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: '#FFCDD2',
  },
  logoutIconWrap: {
    width: 40,
    height: 40,
    borderRadius: 20,
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 14,
  },
  logoutText: {
    fontSize: 16,
    fontWeight: '700',
    color: LOGOUT_RED,
  },
  accountFooter: {
    alignItems: 'center',
    marginTop: 24,
    marginBottom: 8,
  },
  accountFooterTitle: {
    fontSize: 13,
    color: SECONDARY_GREY,
    marginBottom: 6,
  },
  accountFooterLine: {
    fontSize: 13,
    color: SECONDARY_GREY,
    textAlign: 'center',
  },
  accountFooterLink: {
    color: ACCENT_ORANGE,
    fontWeight: '600',
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

export default MyKeepAccountScreen;
