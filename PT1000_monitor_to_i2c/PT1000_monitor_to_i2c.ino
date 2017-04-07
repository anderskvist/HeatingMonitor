#include <Wire.h>

const int S1 = A0;
const int S2 = A1;
const int S3 = A2;

unsigned int S1Value = 0;
unsigned int S2Value = 0;
unsigned int S3Value = 0;

char output[14];

void setup() {
  analogReference(EXTERNAL);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  Serial.begin(9600);

  Wire.begin(8);
  Wire.onRequest(requestEvent);
}

unsigned int analogReadMedian(int pin, int num) {
  int samples[num];

  for(int i = 0; i < num; i++) {
    samples[i] = analogRead(pin);
    delay(1);
  }

  isort(samples, num);
  
  return samples[num/2];
}

void loop() {
  S1Value = analogReadMedian(S1, 101);
  S2Value = analogReadMedian(S2, 21);
  S3Value = analogReadMedian(S3, 21);
  sprintf(output, "%4d%4d%4d", S1Value, S2Value, S3Value);
  //Serial.println(output);
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

