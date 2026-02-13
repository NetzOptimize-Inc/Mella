/**
 * MyKeep logo: official brand icon used in headers across Setup, Devices, Settings, Account.
 */

import React from 'react';
import {Image, StyleSheet} from 'react-native';

const MY_KEEP_LOGO = require('./e7e01b9c-dbc4-4660-8a67-6aae9aabc831.png');

interface MyKeepLogoProps {
  size?: number;
}

const MyKeepLogo: React.FC<MyKeepLogoProps> = ({size = 28}) => {
  return (
    <Image
      source={MY_KEEP_LOGO}
      style={[styles.logo, {width: size, height: size}]}
      resizeMode="contain"
      accessibilityLabel="The Mella"
    />
  );
};

const styles = StyleSheet.create({
  logo: {
    backgroundColor: 'transparent',
  },
});

export default MyKeepLogo;
