#include "ScaleMapper.h"

// Semitone intervals above root for each scale degree
const uint8_t ScaleMapper::MAJOR_INTERVALS[7] = {0, 2, 4, 5, 7, 9, 11};
const uint8_t ScaleMapper::MINOR_INTERVALS[7] = {0, 2, 3, 5, 7, 8, 10};

ScaleMapper::ScaleMapper() : _key(0), _scale(MINOR) {
    _buildSnapTable();
}

void ScaleMapper::setKey(uint8_t key) {
    _key = key % 12;
    _buildSnapTable();
}

void ScaleMapper::setScale(Scale scale) {
    _scale = scale;
    _buildSnapTable();
}

void ScaleMapper::toggleScale() {
    _scale = (_scale == MINOR) ? MAJOR : MINOR;
    _buildSnapTable();
}

int ScaleMapper::snapMidi(int midiNote) const {
    int pc = ((midiNote % 12) + 12) % 12;   // pitch class, always 0-11
    return midiNote + (int)_snapTable[pc];
}

void ScaleMapper::_buildSnapTable() {
    bool inScale[12] = {};
    const uint8_t* intervals = (_scale == MAJOR) ? MAJOR_INTERVALS : MINOR_INTERVALS;

    for (int i = 0; i < 7; i++) {
        inScale[(_key + intervals[i]) % 12] = true;
    }

    for (int pc = 0; pc < 12; pc++) {
        if (inScale[pc]) {
            _snapTable[pc] = 0;
            continue;
        }
        // Search outward; check +delta before -delta so ties snap upward.
        for (int delta = 1; delta <= 6; delta++) {
            if (inScale[(pc + delta) % 12]) {
                _snapTable[pc] = (int8_t)+delta;
                break;
            }
            if (inScale[(pc - delta + 12) % 12]) {
                _snapTable[pc] = (int8_t)-delta;
                break;
            }
        }
    }
}
