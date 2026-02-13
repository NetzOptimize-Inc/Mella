/**
 * Mella app color palette – matches Setup Next button and tab accent.
 * Use these across all screens for a consistent look.
 */
export const colors = {
  // Primary accent (Next button, primary CTAs, active tab)
  accent: '#FF6600',
  // Light accent background (active tab, profile card)
  accentLight: '#FFF0E6',
  // Text
  primary: '#333333',
  secondary: '#666666',
  placeholder: '#999999',
  // Backgrounds
  background: '#F9F9F9',
  surface: '#FFFFFF',
  card: '#F0F0F0',
  // Borders & inactive
  border: '#DDDDDD',
  borderLight: '#E8E8E8',
  // Buttons
  buttonPrimary: '#FF6600',
  buttonSecondary: '#F0F0F0',
  buttonTextOnPrimary: '#FFFFFF',
  buttonTextOnSecondary: '#333333',
  // Status
  success: '#4CAF50',
  error: '#D32F2F',
  errorBg: '#FFEBEE',
  warningBg: '#FFF3CD',
  warningBorder: '#FFC107',
  warningText: '#856404',
  // Overlay
  overlay: 'rgba(0, 0, 0, 0.5)',
} as const;

export type Colors = typeof colors;
