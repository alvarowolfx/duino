//#include <Servo.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

bool debug = false;

int index = 0;

char messageBuffer[12];
char cmd[3];
char pin[3];
char val[4];
char aux[4];

//Servo servo;

bool isNetworkEnabled = false;
// nRF24L01(+) radio attached using Getting Started board 
RF24 radio(7,8);
// Network uses that radio
RF24Network network(radio);
// Address of base node
const uint16_t this_node = 0;
const uint16_t channel = 90;

// Structure of our payload
struct payload_t
{
  unsigned long pin;
  unsigned long state;
};

void setup() {
  Serial.begin(115200);
}

void loop() {
  if(isNetworkEnabled){
    // Pump the network regularly if the module was started
    handleNetworkUpdate();    
  }
  while(Serial.available() > 0) {
    char x = Serial.read();
    if (x == '!') index = 0;      // start
    else if (x == '.') process(); // end
    else messageBuffer[index++] = x;
  }
    

}

/*
 * Deal with a full message and determine function to call
 */
void process() {
  index = 0;

  strncpy(cmd, messageBuffer, 2);
  cmd[2] = '\0';
  strncpy(pin, messageBuffer + 2, 2);
  pin[2] = '\0';

  if (atoi(cmd) > 90) {
    strncpy(val, messageBuffer + 4, 2);
    val[2] = '\0';
    strncpy(aux, messageBuffer + 6, 3);
    aux[3] = '\0';
  } else {
    strncpy(val, messageBuffer + 4, 3);
    val[3] = '\0';
    strncpy(aux, messageBuffer + 7, 3);
    aux[3] = '\0';
  }

  if (debug) {
    Serial.println(messageBuffer);
  }
  int cmdid = atoi(cmd);

  // Serial.println(cmd);
  // Serial.println(pin);
  // Serial.println(val);
  // Serial.println(aux);

  switch(cmdid) {
    case 0:  sm(pin,val);              break;
    case 1:  dw(pin,val);              break;
    case 2:  dr(pin,val);              break;
    case 3:  aw(pin,val);              break;
    case 4:  ar(pin,val);              break;
    case 96: handleNetwork(pin,val,aux); break;
    case 97: handlePing(pin,val,aux);  break;
    /*case 98: handleServo(pin,val,aux); break;*/
    case 99: toggleDebug(val);         break;
    default:                           break;
  }
}

const char* types[3]= {"send","recv","fail"};
const int SEND = 0;
const int RECV = 1;
const int FAIL = 2;
void printNetworkMessage(int type,int state,int node,int pin){    
    char m[30];
    sprintf(m, "rf::%d::%d::%d::%d",types[type],state,node,pin);
    Serial.println(m);    
    free(m);
}

/*
 * Handle nrf comands
 * send
 */
void handleNetwork(char *pin,char *node,char *state){

  int stateVal = atoi(state);
  int to_node = atoi(node);
  
  if(!isNetworkEnabled){
    SPI.begin();
    radio.begin();
    network.begin(/*channel*/ channel,/*node address*/ this_node);
    // Pump the network regularly
    network.update();
    isNetworkEnabled = true;
  }  

  if(to_node == 0){
    char msgInit[9];
    sprintf(msgInit, "rf::init::%s", node);
    Serial.println(msgInit);
    free(msgInit);
    return;
  }

  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }  

  payload_t payload = { p, stateVal };
  RF24NetworkHeader header(/*to node*/ to_node);
  bool ok = network.write(header,&payload,sizeof(payload));

  if(ok){
    printNetworkMessage(SEND,stateVal,to_node,p);
  }else{
    printNetworkMessage(FAIL,stateVal,to_node,p);
  }

}

void handleNetworkUpdate(){
    network.update();
    while(network.available()){
       // If so, grab it and print it out
      RF24NetworkHeader header;
      payload_t payload;
      network.read(header,&payload,sizeof(payload));
      printNetworkMessage(RECV,payload.state,header.from_node,payload.pin);
   }
}

/*
 * Toggle debug mode
 */
void toggleDebug(char *val) {
  if (atoi(val) == 0) {
    debug = false;
    Serial.println("goodbye");
  } else {
    debug = true;
    Serial.println("hello");
  }
}

/*
 * Set pin mode
 */
void sm(char *pin, char *val) {
  if (debug) Serial.println("sm");
  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  if (atoi(val) == 0) {
    pinMode(p, OUTPUT);
  } else {
    pinMode(p, INPUT);
  }
}

/*
 * Digital write
 */
void dw(char *pin, char *val) {
  if (debug) Serial.println("dw");
  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  pinMode(p, OUTPUT);
  if (atoi(val) == 0) {
    digitalWrite(p, LOW);
  } else {
    digitalWrite(p, HIGH);
  }
}

/*
 * Digital read
 */
void dr(char *pin, char *val) {
  if (debug) Serial.println("dr");
  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  pinMode(p, INPUT);
  int oraw = digitalRead(p);
  char m[7];
  sprintf(m, "%02d::%02d", p,oraw);
  Serial.println(m);
}

/*
 * Analog read
 */
void ar(char *pin, char *val) {
  if(debug) Serial.println("ar");
  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  pinMode(p, INPUT); // don't want to sw
  int rval = analogRead(p);
  char m[8];
  sprintf(m, "%s::%03d", pin, rval);
  Serial.println(m);
}

void aw(char *pin, char *val) {
  if(debug) Serial.println("aw");
  int p = getPin(pin);
  pinMode(p, OUTPUT);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  analogWrite(p,atoi(val));
}

int getPin(char *pin) { //Converts to A0-A5, and returns -1 on error
  int ret = -1;
  if(pin[0] == 'A' || pin[0] == 'a') {
    switch(pin[1]) {
      case '0':  ret = A0; break;
      case '1':  ret = A1; break;
      case '2':  ret = A2; break;
      case '3':  ret = A3; break;
      case '4':  ret = A4; break;
      case '5':  ret = A5; break;
      default:             break;
    }
  } else {
    ret = atoi(pin);
    if(ret == 0 && (pin[0] != '0' || pin[1] != '0')) {
      ret = -1;
    }
  }
  return ret;
}

/*
 * Handle Ping commands
 * fire, read
 */
void handlePing(char *pin, char *val, char *aux) {
  if (debug) Serial.println("ss");
  int p = getPin(pin);

  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  Serial.println("got signal");

  // 01(1) Fire and Read
  if (atoi(val) == 1) {
    char m[16];

    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
    delayMicroseconds(2);
    digitalWrite(p, HIGH);
    delayMicroseconds(5);
    digitalWrite(p, LOW);

    Serial.println("ping fired");

    pinMode(p, INPUT);
    sprintf(m, "%s::read::%08d", pin, pulseIn(p, HIGH));
    Serial.println(m);

    delay(50);
  }
}

/*
 * Handle Servo commands
 * attach, detach, write, read, writeMicroseconds, attached
 
void handleServo(char *pin, char *val, char *aux) {
  if (debug) Serial.println("ss");
  int p = getPin(pin);
  if(p == -1) { if(debug) Serial.println("badpin"); return; }
  Serial.println("signal: servo");

  // 00(0) Detach
  if (atoi(val) == 0) {
    servo.detach();
    char m[12];
    sprintf(m, "%s::detached", pin);
    Serial.println(m);

  // 01(1) Attach
  } else if (atoi(val) == 1) {
    // servo.attach(p, 750, 2250);
    servo.attach(p);
    char m[12];
    sprintf(m, "%s::attached", pin);
    Serial.println(m);

  // 02(2) Write
  } else if (atoi(val) == 2) {
    Serial.println("writing to servo");
    Serial.println(atoi(aux));
    // Write to servo
    servo.write(atoi(aux));
    delay(15);


  // 03(3) Read
  } else if (atoi(val) == 3) {
    Serial.println("reading servo");
    int sval = servo.read();
    char m[13];
    sprintf(m, "%s::read::%03d", pin, sval);
    Serial.println(m);
  }
}
*/
