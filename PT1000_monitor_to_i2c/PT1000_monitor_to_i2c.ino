#include <Wire.h>

const int S1 = A0;
const int S2 = A1;
const int S3 = A2;

int S1Value = 0;
int S2Value = 0;
int S3Value = 0;

char output[14];

void setup() {
  analogReference(DEFAULT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  Wire.begin(8);
  Wire.onRequest(requestEvent);
}

int analogReadMedian(int pin) {
  int num = 21;
  int samples[num];

  for(int i = 0; i < num; i++) {
    samples[i] = analogRead(pin);
  }

  isort(samples, num);
  
  return samples[10];
}

void loop() {
  S1Value = analogReadMedian(S1);
  S2Value = analogReadMedian(S2);
  S3Value = analogReadMedian(S3);
  sprintf(output, "%4d;%4d;%4d", S1Value, S2Value, S3Value);
  delay(500);
}

void requestEvent() {
  Wire.write(output);
}

/* http://forum.arduino.cc/index.php?topic=49059.msg350709#msg350709 */
//Bubble sort my ar*e
void isort(int *a, int n)
{
 for (int i = 1; i < n; ++i)
 {
   int j = a[i];
   int k;
   for (k = i - 1; (k >= 0) && (j < a[k]); k--)
   {
     a[k + 1] = a[k];
   }
   a[k + 1] = j;
 }
}

