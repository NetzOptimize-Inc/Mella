/**
 * MyKeep-style setup screen
 * Matches the MyKeep setup flow UI: header, progress stepper, step content, Next CTA, bottom nav.
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
const BG_OFF_WHITE = '#F9F9F9';
const BG_WHITE = '#FFFFFF';
const INACTIVE_STEP = '#DDDDDD';
const WHITE = '#FFFFFF';

export type MyKeepTab = 'setup' | 'devices' | 'settings' | 'account';

interface MyKeepSetupScreenProps {
  onNext?: () => void;
  onTab?: (tab: MyKeepTab) => void;
  activeTab?: MyKeepTab;
}

const MyKeepSetupScreen: React.FC<MyKeepSetupScreenProps> = ({
  onNext,
  onTab,
  activeTab = 'setup',
}) => {
  return (
    <View style={styles.outer}>
      {/* Header */}
      <View style={styles.header}>
        <View style={styles.logoIcon}>
          <MyKeepLogo size={28} />
        </View>
        <Text style={styles.headerTitle}>MyKeep</Text>
      </View>

      {/* Progress stepper */}
      <View style={styles.stepper}>
        <View style={[styles.stepSegment, styles.stepActive]} />
        <View style={[styles.stepSegment, styles.stepInactive]} />
        <View style={[styles.stepSegment, styles.stepInactive]} />
      </View>

      {/* Main content */}
      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}>
        <View style={styles.content}>
          <View style={styles.powerIconWrap}>
            <MaterialCommunityIcons name="power" size={64} color={ACCENT_ORANGE} />
          </View>
          <Text style={styles.stepTitle}>Switch On the Product</Text>
          <Text style={styles.stepDescription}>
            Press the power button on your MyKeep box to turn it on.
          </Text>
        </View>
      </ScrollView>

      {/* Next button - fixed at bottom above nav */}
      <View style={styles.nextButtonWrap}>
        <TouchableOpacity
          style={styles.nextButton}
          onPress={onNext}
          activeOpacity={0.85}>
          <Text style={styles.nextButtonText}>Next</Text>
        </TouchableOpacity>
      </View>

      {/* Bottom navigation */}
      <View style={styles.bottomNav}>
        <TouchableOpacity
          style={[styles.navItem, activeTab === 'setup' && styles.navItemActive]}
          onPress={() => onTab?.('setup')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons
            name="lightning-bolt"
            size={24}
            color={activeTab === 'setup' ? ACCENT_ORANGE : PRIMARY_GREY}
            style={styles.navIcon}
          />
          <Text
            style={[
              styles.navLabel,
              activeTab === 'setup' && styles.navLabelActive,
            ]}>
            Setup
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, activeTab === 'devices' && styles.navItemActive]}
          onPress={() => onTab?.('devices')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons
            name="calendar-clock"
            size={24}
            color={activeTab === 'devices' ? ACCENT_ORANGE : PRIMARY_GREY}
            style={styles.navIcon}
          />
          <Text
            style={[
              styles.navLabel,
              activeTab === 'devices' && styles.navLabelActive,
            ]}>
            Scheduler
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, activeTab === 'settings' && styles.navItemActive]}
          onPress={() => onTab?.('settings')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons
            name="cog"
            size={24}
            color={activeTab === 'settings' ? ACCENT_ORANGE : PRIMARY_GREY}
            style={styles.navIcon}
          />
          <Text
            style={[
              styles.navLabel,
              activeTab === 'settings' && styles.navLabelActive,
            ]}>
            Settings
          </Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.navItem, activeTab === 'account' && styles.navItemActive]}
          onPress={() => onTab?.('account')}
          activeOpacity={0.7}>
          <MaterialCommunityIcons
            name="account"
            size={24}
            color={activeTab === 'account' ? ACCENT_ORANGE : PRIMARY_GREY}
            style={styles.navIcon}
          />
          <Text
            style={[
              styles.navLabel,
              activeTab === 'account' && styles.navLabelActive,
            ]}>
            Account
          </Text>
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
  stepper: {
    flexDirection: 'row',
    height: 4,
    backgroundColor: BG_WHITE,
  },
  stepSegment: {
    flex: 1,
    height: '100%',
  },
  stepActive: {
    backgroundColor: ACCENT_ORANGE,
  },
  stepInactive: {
    backgroundColor: INACTIVE_STEP,
  },
  scroll: {
    flex: 1,
    backgroundColor: BG_OFF_WHITE,
  },
  scrollContent: {
    paddingHorizontal: 24,
    paddingTop: 48,
    paddingBottom: 24,
  },
  content: {
    alignItems: 'center',
    marginBottom: 40,
  },
  powerIconWrap: {
    marginBottom: 28,
  },
  stepTitle: {
    fontSize: 22,
    fontWeight: '700',
    color: PRIMARY_GREY,
    textAlign: 'center',
    marginBottom: 12,
    paddingHorizontal: 16,
  },
  stepDescription: {
    fontSize: 16,
    color: SECONDARY_GREY,
    textAlign: 'center',
    lineHeight: 24,
    paddingHorizontal: 8,
  },
  nextButtonWrap: {
    backgroundColor: BG_OFF_WHITE,
    paddingHorizontal: 24,
    paddingTop: 16,
    paddingBottom: 16,
    borderTopWidth: StyleSheet.hairlineWidth,
    borderTopColor: INACTIVE_STEP,
  },
  nextButton: {
    backgroundColor: ACCENT_ORANGE,
    paddingVertical: 16,
    paddingHorizontal: 24,
    borderRadius: 10,
    alignItems: 'center',
    justifyContent: 'center',
  },
  nextButtonText: {
    color: WHITE,
    fontSize: 17,
    fontWeight: '700',
  },
  bottomNav: {
    flexDirection: 'row',
    backgroundColor: BG_OFF_WHITE,
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
    backgroundColor: '#FFF0E6',
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

export default MyKeepSetupScreen;
