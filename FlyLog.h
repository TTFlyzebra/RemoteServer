//
// Created by FlyZebra on 2020/7/20 0020.
//

#ifndef FLYLOG_H
#define FLYLOG_H

#include <stdio.h>


#define FLOGD(...) printf(__VA_ARGS__);printf("\n")
#define FLOGI(...) printf(__VA_ARGS__);printf("\n")
#define FLOGW(...) printf(__VA_ARGS__);printf("\n")
#define FLOGE(...) printf(__VA_ARGS__);printf("\n")

#endif //FLYLOG_H