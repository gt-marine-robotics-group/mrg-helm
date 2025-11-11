#pragma once

/*
When building you can choose between either BOARD_PRESTO or BOARD_SENSOR
and this will set build flags so we can have the firmware of both boards
in one platformio project
*/
#include "robosub.pb.h"


#if defined(BOARD_PRESTO) && BOARD_PRESTO
  constexpr BoardId kBoardId = BoardId::BoardId_BOARD_PRESTO_ID;
#elif defined(BOARD_SENSOR) && BOARD_SENSOR
  constexpr BoardId kBoardId = BoardId::BoardId_BOARD_SENSOR_ID;
#endif

#if defined(BOARD_PRESTO) && BOARD_PRESTO
    /*
    Add in flags here for all the components that are on the presto_board
    we will wrap all the drivers with these flags to be able to activate /
    deactive them.
    
    If there is a component which we plan to have but isn't electrically 
    hooked up you can also comment it out here and the code will ignore 
    everything to do with that sensor
    */
    #define HAS_MOTORS 1
    #define HAS_INDICATOR_LED 1
    #define HAS_E_STOP 1
    #define HAS_AUTONOMY_SWITCH 1

#elif defined(BOARD_SENSOR) && BOARD_SENSOR
    /*
    Add in flags here for all the components that are on the _board
    we will wrap all the drivers with these flags to be able to activate /
    deactive them.
    
    If there is a component which we plan to have but isn't electrically 
    hooked up you can also comment it out here and the code will ignore 
    everything to do with that sensor
    */
    #define HAS_POWER_SENSING 1
    #define HAS_PRESSURE_SENSING 1

#else
  #error "No board macro defined: set BOARD_PRESTO or BOARD_SENSOR in platformio.ini"
#endif

#if (BOARD_PRESTO && BOARD_SENSOR)
  #error "Both BOARD_PRESTO and BOARD_SENSOR defined as true"
#endif
