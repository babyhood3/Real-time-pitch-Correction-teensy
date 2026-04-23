#pragma once
#include <stdint.h>

// Maps a detected MIDI note to the nearest allowed note in the selected
// musical key and scale.  The snap table is rebuilt only on key/scale change
// so snapMidi() is O(1) at runtime.

class ScaleMapper {
public:
    enum Scale { MINOR = 0, MAJOR = 1 };

    ScaleMapper();

    void    setKey(uint8_t key);     // 0=C, 1=C#, 2=D, ... 11=B
    void    setScale(Scale scale);
    void    toggleScale();

    uint8_t getKey()   const { return _key; }
    Scale   getScale() const { return _scale; }

    // Snap midiNote to nearest in-scale degree (integer MIDI, any octave).
    // Returns the snapped MIDI note.  Ties prefer the upper note.
    int snapMidi(int midiNote) const;

private:
    static const uint8_t MAJOR_INTERVALS[7];
    static const uint8_t MINOR_INTERVALS[7];

    uint8_t _key;
    Scale   _scale;

    // For each pitch class 0-11: semitone offset to reach nearest in-scale note.
    // Example: if C# is not in C major, _snapTable[1] = -1 (snap down to C).
    int8_t _snapTable[12];

    void _buildSnapTable();
};
