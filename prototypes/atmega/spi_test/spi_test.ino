

// Initialize SPI Slave Device
void spi_init_slave (void)
{
  //DDRB = (1 << 6);   //MISO as OUTPUT
  SPCR = (1 << SPE); //Enable SPI

  pinMode(MISO, OUTPUT);
  pinMode(7, OUTPUT);
}


//Function to send and receive data for both master and slave
inline unsigned char spi_tranceiver (unsigned char data)
{
  // Load data into the buffer
  SPDR = data;

  //Wait until transmission complete
  digitalWrite(7, LOW);
  while (!(SPSR & (1 << SPIF) ));
  digitalWrite(7, HIGH);

  // Return received data
  return (SPDR);
}

static const char sendBuffer[] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };
static int sendBufferSize = 10;
static int bufferPtr = 0;
static unsigned char recvChar = 0;

void setup() {
  // put your setup code here, to run once:
  spi_init_slave();
}

void loop() {
  // put your main code here, to run repeatedly:
  recvChar = spi_tranceiver(sendBuffer[bufferPtr]);
  bufferPtr = (bufferPtr+1) % sendBufferSize;
}

