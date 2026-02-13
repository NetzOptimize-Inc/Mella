/**
 * Weekly scheduler UI: Device ID, day enable toggles, start/end times (01-24), Schedule button.
 * Integrates with MQTT broker to publish schedule commands.
 */

import React, {useState, useEffect, useRef} from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Switch,
  Modal,
  FlatList,
  Pressable,
  Alert,
  ActivityIndicator,
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';
import MyKeepLogo from './MyKeepLogo';
import { colors as themeColors } from '../theme/colors';
// @ts-ignore - paho-mqtt types
import {Client} from 'paho-mqtt';

const ACCENT_ORANGE = '#FF6600';
const PRIMARY_GREY = '#333333';
const SECONDARY_GREY = '#666666';
const BG_WHITE = '#FFFFFF';
const BG_OFF_WHITE = '#F9F9F9';
const BORDER_COLOR = '#DDDDDD';

const DAYS = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];

const HOUR_OPTIONS = Array.from({length: 24}, (_, i) => {
  const h = i + 1;
  const label = h === 24 ? '24:00' : `${String(h).padStart(2, '0')}:00`;
  return {value: h, label};
});

export interface DaySchedule {
  dayIndex: number;
  dayLabel: string;
  enabled: boolean;
  startHour: number;
  endHour: number;
}

interface SchedulerScreenProps {
  onBack: () => void;
  onSchedule?: (deviceId: string, schedule: DaySchedule[]) => void;
  initialDeviceId?: string;
  /** When true (default), show back button. When false, e.g. embedded in Scheduler tab, show logo only. */
  showBackButton?: boolean;
}

const SchedulerScreen: React.FC<SchedulerScreenProps> = ({
  onBack,
  onSchedule,
  initialDeviceId = 'df72',
  showBackButton = true,
}) => {
  const [deviceId, setDeviceId] = useState(initialDeviceId);
  const [schedule, setSchedule] = useState<DaySchedule[]>(() =>
    DAYS.map((dayLabel, dayIndex) => ({
      dayIndex,
      dayLabel,
      enabled: false,
      startHour: 1,
      endHour: 1,
    })),
  );
  const [timeModal, setTimeModal] = useState<{
    visible: boolean;
    rowIndex: number;
    field: 'start' | 'end';
  } | null>(null);
  const [mqttConnected, setMqttConnected] = useState(false);
  const [isPublishing, setIsPublishing] = useState(false);
  const [logMessages, setLogMessages] = useState<string[]>([]);
  const mqttClientRef = useRef<Client | null>(null);
  const logScrollViewRef = useRef<ScrollView | null>(null);

  // MQTT connection using Paho MQTT
  useEffect(() => {
    const clientId = `mobile_${Math.random().toString(16).substr(2, 8)}`;
    const host = 'broker.hivemq.com';
    const port = 8884;
    const path = '/mqtt';

    try {
      const client = new Client(host, Number(port), path, clientId);

      client.onConnectionLost = (responseObject: any) => {
        setMqttConnected(false);
        addLog(`MQTT connection lost: ${responseObject.errorMessage || 'Unknown'}`);
      };

      client.onMessageArrived = (message: any) => {
        // Handle incoming messages if needed
      };

      client.connect({
        onSuccess: () => {
          setMqttConnected(true);
          addLog(`MQTT connected. Client ID: ${clientId}`);
        },
        onFailure: (error: any) => {
          setMqttConnected(false);
          addLog(`MQTT connection failed: ${error.errorMessage || 'Unknown error'}`);
        },
        useSSL: true,
        cleanSession: true,
      });

      mqttClientRef.current = client;

      return () => {
        if (client && client.isConnected()) {
          client.disconnect();
        }
      };
    } catch (error: any) {
      addLog(`MQTT initialization failed: ${error?.message || 'Unknown error'}`);
    }
  }, []);

  const addLog = (message: string) => {
    setLogMessages(prev => {
      const newLogs = [...prev.slice(-9), message];
      // Auto-scroll to bottom after a short delay
      setTimeout(() => {
        logScrollViewRef.current?.scrollToEnd({animated: true});
      }, 100);
      return newLogs;
    });
  };

  const updateDay = (index: number, patch: Partial<DaySchedule>) => {
    setSchedule(prev =>
      prev.map((row, i) => (i === index ? {...row, ...patch} : row)),
    );
  };

  const handleTimeSelect = (hour: number) => {
    if (!timeModal) return;
    const {rowIndex, field} = timeModal;
    updateDay(rowIndex, field === 'start' ? {startHour: hour} : {endHour: hour});
    setTimeModal(null);
  };

  const handleSchedule = () => {
    const trimmedDeviceId = deviceId.trim();
    if (!trimmedDeviceId) {
      Alert.alert('Error', 'Please enter Device ID');
      return;
    }

    if (!mqttConnected || !mqttClientRef.current || !mqttClientRef.current.isConnected()) {
      Alert.alert('Error', 'MQTT not connected. Please wait...');
      return;
    }

    // Check if at least one day is enabled
    const hasEnabledDay = schedule.some(day => day.enabled);
    if (!hasEnabledDay) {
      Alert.alert('Error', 'Please enable at least one day to schedule');
      return;
    }

    setIsPublishing(true);

    // Build days object matching PHP format - only include enabled days
    const daysObj: Record<number, any> = {};

    schedule.forEach(day => {
      if (day.enabled) {
        daysObj[day.dayIndex] = {
          en: true,
          sh: day.startHour,
          sm: 0,
          eh: day.endHour,
          em: 0,
        };
      } else {
        daysObj[day.dayIndex] = {en: false};
      }
    });

    const payload = {
      version: 1,
      enabled: true,
      days: daysObj,
    };

    const topic = `mella/${trimmedDeviceId}/cmd/schedule`;
    const message = JSON.stringify(payload);

    try {
      if (mqttClientRef.current) {
        mqttClientRef.current.publish(topic, message, 0);
        
        setIsPublishing(false);
        addLog(`✓ Published to ${topic}`);
        addLog(`Payload:`);
        addLog(JSON.stringify(payload, null, 2));
        addLog(`--- Schedule sent successfully ---`);
        // Show success alert but don't navigate back automatically
        Alert.alert('Success', 'Schedule published successfully');
      }
    } catch (err: any) {
      setIsPublishing(false);
      addLog(`✗ Publish failed: ${err?.message || 'Unknown error'}`);
      Alert.alert('Error', `Failed to publish schedule: ${err?.message || 'Unknown error'}`);
    }
  };

  const openTimeModal = (rowIndex: number, field: 'start' | 'end') => {
    setTimeModal({visible: true, rowIndex, field});
  };


  return (
    <View style={styles.outer}>
      <View style={styles.header}>
        {showBackButton ? (
          <TouchableOpacity onPress={onBack} style={styles.backBtn} hitSlop={12}>
            <MaterialCommunityIcons name="arrow-left" size={24} color={PRIMARY_GREY} />
          </TouchableOpacity>
        ) : (
          <View style={styles.backBtn}>
            <MyKeepLogo size={28} />
          </View>
        )}
        <Text style={styles.headerTitle}>The Mella</Text>
      </View>

      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}>
        <Text style={styles.screenTitle}>The Mella – Weekly Scheduler</Text>

        <View style={styles.fieldRow}>
          <Text style={styles.label}>Device ID:</Text>
          <TextInput
            style={styles.input}
            value={deviceId}
            onChangeText={setDeviceId}
            placeholder="Device ID"
            placeholderTextColor={SECONDARY_GREY}
          />
        </View>

        <View style={styles.tableWrap}>
          <View style={styles.tableHeader}>
            <Text style={[styles.cell, styles.cellDay]}>Day</Text>
            <Text style={[styles.cell, styles.cellEnable]}>Enable</Text>
            <Text style={[styles.cell, styles.cellTime]}>Start (01-24)</Text>
            <Text style={[styles.cell, styles.cellTime]}>End (01-24)</Text>
          </View>
          {schedule.map((row, index) => (
            <View key={row.dayLabel} style={styles.tableRow}>
              <Text style={[styles.cell, styles.cellDay]}>{row.dayLabel}</Text>
              <View style={[styles.cell, styles.cellEnable]}>
                <Switch
                  value={row.enabled}
                  onValueChange={v => updateDay(index, {enabled: v})}
                  trackColor={{false: BORDER_COLOR, true: ACCENT_ORANGE}}
                  thumbColor={BG_WHITE}
                />
              </View>
              <TouchableOpacity
                style={[styles.cell, styles.cellTime]}
                onPress={() => openTimeModal(index, 'start')}>
                <Text style={styles.timeText}>
                  {row.startHour === 24 ? '24:00' : `${String(row.startHour).padStart(2, '0')}:00`}
                </Text>
              </TouchableOpacity>
              <TouchableOpacity
                style={[styles.cell, styles.cellTime]}
                onPress={() => openTimeModal(index, 'end')}>
                <Text style={styles.timeText}>
                  {row.endHour === 24 ? '24:00' : `${String(row.endHour).padStart(2, '0')}:00`}
                </Text>
              </TouchableOpacity>
            </View>
          ))}
        </View>

        <TouchableOpacity
          style={[
            styles.scheduleButton,
            (!mqttConnected || isPublishing) && styles.scheduleButtonDisabled,
          ]}
          onPress={handleSchedule}
          disabled={!mqttConnected || isPublishing}
          activeOpacity={0.85}>
          {isPublishing ? (
            <ActivityIndicator color={BG_WHITE} />
          ) : (
            <Text style={styles.scheduleButtonText}>Schedule</Text>
          )}
        </TouchableOpacity>

        {logMessages.length > 0 && (
          <View style={styles.logContainer}>
            <Text style={styles.logTitle}>Log:</Text>
            <ScrollView
              ref={logScrollViewRef}
              style={styles.logScroll}
              nestedScrollEnabled={true}>
              {logMessages.map((msg, idx) => (
                <Text key={idx} style={styles.logText}>
                  {msg}
                </Text>
              ))}
            </ScrollView>
          </View>
        )}

        {!mqttConnected && (
          <View style={styles.statusContainer}>
            <Text style={styles.statusText}>
              Connecting to MQTT broker...
            </Text>
          </View>
        )}
      </ScrollView>

      <Modal
        visible={timeModal?.visible ?? false}
        transparent
        animationType="fade">
        <Pressable
          style={styles.modalOverlay}
          onPress={() => setTimeModal(null)}>
          <View style={styles.modalContent}>
            <Text style={styles.modalTitle}>
              {timeModal?.field === 'start' ? 'Start time' : 'End time'}
            </Text>
            <FlatList
              data={HOUR_OPTIONS}
              keyExtractor={item => item.label}
              style={styles.modalList}
              renderItem={({item}) => (
                <TouchableOpacity
                  style={styles.modalItem}
                  onPress={() => handleTimeSelect(item.value)}>
                  <Text style={styles.modalItemText}>{item.label}</Text>
                </TouchableOpacity>
              )}
            />
            <TouchableOpacity
              style={styles.modalCancel}
              onPress={() => setTimeModal(null)}>
              <Text style={styles.modalCancelText}>Cancel</Text>
            </TouchableOpacity>
          </View>
        </Pressable>
      </Modal>
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
    paddingHorizontal: 16,
    paddingVertical: 14,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: BORDER_COLOR,
  },
  backBtn: {
    marginRight: 12,
  },
  headerTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: PRIMARY_GREY,
  },
  scroll: {
    flex: 1,
  },
  scrollContent: {
    padding: 20,
    paddingBottom: 40,
  },
  screenTitle: {
    fontSize: 18,
    fontWeight: '700',
    color: PRIMARY_GREY,
    marginBottom: 16,
  },
  fieldRow: {
    marginBottom: 20,
  },
  label: {
    fontSize: 14,
    color: PRIMARY_GREY,
    marginBottom: 6,
  },
  input: {
    backgroundColor: BG_WHITE,
    borderWidth: 1,
    borderColor: BORDER_COLOR,
    borderRadius: 8,
    paddingHorizontal: 12,
    paddingVertical: 10,
    fontSize: 16,
    color: PRIMARY_GREY,
  },
  tableWrap: {
    backgroundColor: BG_WHITE,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: BORDER_COLOR,
    overflow: 'hidden',
    marginBottom: 24,
  },
  tableHeader: {
    flexDirection: 'row',
    backgroundColor: BG_OFF_WHITE,
    borderBottomWidth: 1,
    borderBottomColor: BORDER_COLOR,
  },
  tableRow: {
    flexDirection: 'row',
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: BORDER_COLOR,
    alignItems: 'center',
  },
  cell: {
    paddingVertical: 10,
    paddingHorizontal: 6,
    minHeight: 44,
    justifyContent: 'center',
  },
  cellDay: {
    width: 48,
    fontSize: 13,
    fontWeight: '600',
    color: PRIMARY_GREY,
  },
  cellEnable: {
    width: 72,
    alignItems: 'center',
  },
  cellTime: {
    flex: 1,
    alignItems: 'center',
  },
  timeText: {
    fontSize: 13,
    color: PRIMARY_GREY,
  },
  scheduleButton: {
    backgroundColor: ACCENT_ORANGE,
    paddingVertical: 14,
    paddingHorizontal: 24,
    borderRadius: 8,
    alignSelf: 'flex-start',
  },
  scheduleButtonText: {
    color: BG_WHITE,
    fontSize: 16,
    fontWeight: '700',
  },
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.5)',
    justifyContent: 'center',
    alignItems: 'center',
    padding: 24,
  },
  modalContent: {
    backgroundColor: BG_WHITE,
    borderRadius: 12,
    padding: 16,
    width: '100%',
    maxHeight: 320,
  },
  modalTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: PRIMARY_GREY,
    marginBottom: 12,
  },
  modalList: {
    maxHeight: 220,
  },
  modalItem: {
    paddingVertical: 12,
    paddingHorizontal: 16,
  },
  modalItemText: {
    fontSize: 16,
    color: PRIMARY_GREY,
  },
  modalCancel: {
    marginTop: 12,
    paddingVertical: 10,
    alignItems: 'center',
  },
  modalCancelText: {
    fontSize: 16,
    color: ACCENT_ORANGE,
    fontWeight: '600',
  },
  scheduleButtonDisabled: {
    opacity: 0.5,
  },
  logContainer: {
    marginTop: 20,
    backgroundColor: BG_WHITE,
    borderRadius: 8,
    padding: 12,
    borderWidth: 1,
    borderColor: BORDER_COLOR,
    maxHeight: 200,
  },
  logTitle: {
    fontSize: 14,
    fontWeight: '600',
    color: PRIMARY_GREY,
    marginBottom: 8,
  },
  logScroll: {
    maxHeight: 170,
  },
  logText: {
    fontSize: 12,
    fontFamily: 'monospace',
    color: SECONDARY_GREY,
    marginBottom: 2,
  },
  statusContainer: {
    marginTop: 12,
    padding: 12,
    backgroundColor: themeColors.warningBg,
    borderRadius: 8,
  },
  statusText: {
    fontSize: 13,
    color: themeColors.warningText,
  },
});

export default SchedulerScreen;
