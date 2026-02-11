declare module 'react-native-vector-icons/MaterialCommunityIcons' {
  import {Component} from 'react';

  export interface IconProps {
    name: string;
    size?: number;
    color?: string;
    style?: object;
  }

  export default class MaterialCommunityIcons extends Component<IconProps> {}
}
