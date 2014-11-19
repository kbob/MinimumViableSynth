//
//  main.cpp
//  One Note
//
//  Created by Bob Miller on 11/17/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#include <iostream>

#include "Envelope.h"
#include "Oscillator.h"

class OneNote {

public:
    OneNote();
    size_t render(Float32 *outbuf, size_t count);

private:
    Oscillator mOsc1;
    Envelope mAmpEnv;
};

OneNote::OneNote()
{
    mOsc1.initialize(44100.0, Oscillator::OT_Saw);
    mAmpEnv.initialize(44100.0, 0.1, 0.2, 0.1, 0.3);
}

size_t OneNote::render(Float32 *outbuf, size_t count)
{
    const size_t chunk_size = 512;
    size_t note_count;
    Float32 ampbuf[chunk_size], osc1buf[chunk_size];
    bool released = false;
    for (size_t i = 0; i < count; i++)
        outbuf[i] = -0.5;
    for (note_count = 0; note_count < count; ) {
        size_t chunk_count = count - note_count;
        if (chunk_count > chunk_size)
            chunk_count = chunk_size;
        UInt32 end = mAmpEnv.generate(ampbuf, (UInt32)chunk_count);
        if (end != 0xFFFFFFFF)
            chunk_count = end;
        if (note_count > count / 2 && !released) {
            mAmpEnv.release();
            released = true;
        }
        mOsc1.generate(440.0, osc1buf, (UInt32)chunk_count);
        for (size_t i = 0; i < chunk_count; i++)
            outbuf[note_count + i] = osc1buf[i] * ampbuf[i];
//        for (size_t i = 0; i < chunk_count; i++)
//            outbuf[note_count + i] = ampbuf[i];
//        for (size_t i = 0; i < chunk_count; i++)
//            outbuf[note_count + i] = osc1buf[i];
        note_count += chunk_count;
        if (end != 0xFFFFFFFF)
            break;
    }
    return note_count;
}

void save_to_foo(Float32 *buf, size_t count)
{
    FILE *f = fopen("/tmp/foo", "w");
    for (size_t i = 0; i < 5000; i++) {
        fprintf(f, "0\n");
    }
    for (size_t i = 0; i < count; i++) {
        Float32 x = buf[i];
        if (isnan(x) || isinf(x))
            x = 0;
        fprintf(f, "%g\n", x);

    }
    fprintf(f, "end\n");
    fclose(f);
}

int main(int argc, const char * argv[]) {
    // insert code here...
    std::cout << "Hello, World!\n";
    OneNote a;
    size_t count = 44100;
    Float32 note[count];
    count = a.render(note, count);
    save_to_foo(note, count);
    std::cout << "Good bye, World!\n";
    return 0;
}
