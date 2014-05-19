import shapes3d.*;
import shapes3d.utils.*;
import shapes3d.animation.*;
import processing.serial.*;

Serial serial;
Box box;

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

public void setup() {
  size(400, 400, P3D);
  createBox();
  setupSerial();
}

void setupSerial()
{
  // DEBUG - Print the connected serial devices
  println("All serial devices: " + Serial.list());
  println("Selected device: " + Serial.list()[5]);

  // Baud Rates: 9600 - 57600 - 115200
  serial = new Serial(this, Serial.list()[5], 9600);
  serial.bufferUntil('\n');
}

void createBox()
{
  box = new Box(this);
  String[] faces = new String[] {
    "front.png", "back.png", "right.png",
    "left.png", "bottom.png", "top.png"
  };
  box.setTextures(faces);
  box.fill(color(255));
  box.stroke(color(190));
  box.strokeWeight(1.2f);
  box.setSize(100, 50, 150);
  box.drawMode(S3D.TEXTURE | S3D.WIRE);
}

public void draw() {
  background(20);

  pushMatrix();
  camera(0, 0, 300, 0, 0, 0, 0, 1, 0);
  rotateX(radians(roll)*-1);
  rotateZ(radians(pitch)*-1);
  rotateY(radians(yaw));
  box.draw();
  popMatrix();
}

void serialEvent (Serial serial) {
  try
  {
//    String packet = new String(serial.readBytesUntil('\n'));
//    println("Packet: " + packet);
    
    serial.readBytesUntil('R');
    roll = readFloatUntil(serial, '\t');
    pitch = readFloatUntil(serial, '\t');
    yaw = readFloatUntil(serial, '\n');
    println("Roll:"+roll+" Pitch:"+pitch+" Yaw:"+yaw);

    serial.clear();
  } catch(Exception e) {
    println( e );
  }
}

public float readFloatUntil(Serial serial, int inByte) {
  byte temp[] = serial.readBytesUntil(inByte);
  println("bytes = " + temp.length);
  if (temp == null) {
    return 0.0;
  } else {
    float value = get4bytesFloat(temp, 0);
    return value;
  }
}

float get4bytesFloat(byte[] data, int offset) {
//  println("bit4: " + hex(data[offset+3]) );
  String hexint = hex(data[offset+3])+hex(data[offset+2])+hex(data[offset+1])+hex(data[offset]);
  println("unhexed: " + unhex(hexint) );
  return Float.intBitsToFloat(unhex(hexint));
}
