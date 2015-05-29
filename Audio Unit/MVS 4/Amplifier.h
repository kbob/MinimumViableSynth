//
//  Amplifier.h
//  MVS 4
//
//  Created by Bob Miller on 12/31/14.
//  Copyright (c) 2014 kbobsoft.com. All rights reserved.
//

#ifndef __MVS_4__Amplifier__
#define __MVS_4__Amplifier__

class Amplifier {

public:

    Amplifier    ();

    void initialize   (float sample_rate);

    void generate_sum (float const *signal,
                       float const *gain,
                       float       *out,
                       size_t       nsamp);
    
};

#endif /* defined(__MVS_4__Amplifier__) */
