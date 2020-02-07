#include "../DataStructures/PriorityQueue.h"
#include <stdlib.h>
#include <stdio.h>


int main(void) {
  PriorityQueue<double> mode_bins;
  double dubs[20] = {0.54, 0.10, 0.68, 0.54, 0.54, \
                     0.10, 0.17, 0.67, 0.54, 0.09, \
                     0.57, 0.15, 0.68, 0.54, 0.67, \
                     0.11, 0.10, 0.64, 0.54, 0.09};
   
    
  for (int i = 0; i < 20; i++) {
      if (mode_bins.contains(dubs[i])) {
        mode_bins.incrementPriority(dubs[i]);
      }
      else {
        mode_bins.insert(dubs[i], 1);
      }
  }
  
  double temp_double = 0;  
  double most_common = mode_bins.get();
  int stat_mode      = mode_bins.getPriority(most_common);
  printf("Most common:  %lf\n", most_common);
  printf("Mode:         %d\n\n",  stat_mode);
  
  // Now let's print a simple histogram...
  while (mode_bins.hasNext()) {
    temp_double = mode_bins.get();
    stat_mode = mode_bins.getPriority(temp_double);
    printf("%lf\t", temp_double);
    for (int n = 0; n < stat_mode; n++) {
      printf("*");
    }
    printf("  (%d)\n", stat_mode);
    mode_bins.dequeue();
  }
}

