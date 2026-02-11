/**
 * Weekly scheduler UI: Device ID, day enable toggles, start/end times (01-24), Schedule button.
 */

import React, {useState} from 'react';
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
} from 'react-native';
import MaterialCommunityIcons from 'react-native-vector-icons/MaterialCommunityIcons';

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
}

const SchedulerScreen: React.FC<SchedulerScreenProps> = ({
  onBack,
  onSchedule,
  initialDeviceId = 'df72',
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
    onSchedule?.(deviceId, schedule);
    onBack();
  };

  const openTimeModal = (rowIndex: number, field: 'start' | 'end') => {
    setTimeModal({visible: true, rowIndex, field});
  };


  return (
    <View style={styles.outer}>
      <View style={styles.header}>
        <TouchableOpacity onPress={onBack} style={styles.backBtn} hitSlop={12}>
          <MaterialCommunityIcons name="arrow-left" size={24} color={PRIMARY_GREY} />
        </TouchableOpacity>
        <Text style={styles.headerTitle}>Weekly Scheduler</Text>
      </View>

      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
        showsVerticalScrollIndicator={true}>
        <Text style={styles.screenTitle}>MyKeep – Weekly Scheduler</Text>

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
          style={styles.scheduleButton}
          onPress={handleSchedule}
          activeOpacity={0.85}>
          <Text style={styles.scheduleButtonText}>Schedule</Text>
        </TouchableOpacity>
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
});

export default SchedulerScreen;
