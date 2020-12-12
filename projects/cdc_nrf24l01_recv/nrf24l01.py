import serial
import struct

class Board:
    OP_READ_PORT    = 0 << 5
    OP_WRITE_PORT   = 1 << 5
    OP_READ_PIN     = 2 << 5
    OP_WRITE_PIN    = 3 << 5
    OP_SPI_RW       = 4 << 5
    def __init__(self, ser):
        self.ser = ser;

    def read_pin(self, pin):
        cmd = Board.OP_READ_PIN | (pin & 0b11111)
        self.ser.write(struct.pack('B', cmd))
        return struct.unpack('B', self.ser.read())[0] == 1

    def write_pin(self, pin, state):
        state = 1 << 4 if state else 0
        cmd = Board.OP_WRITE_PIN | state | (pin & 0x0F)
        self.ser.write(struct.pack('B', cmd))

    def spi_rw(self, byte):
        if isinstance(byte, int):
            byte = struct.pack('B', byte)
        self.ser.write(struct.pack('B', Board.OP_SPI_RW))
        self.ser.write(byte)
        return struct.unpack('B', self.ser.read())[0]


PIN_LED     = 0
PIN_IRQ     = 1
PIN_CE      = 2
PIN_CSN     = 3
PIN_MOSI    = 4
PIN_MISO    = 5
PIN_SCK     = 6

RF24_CMD_R_REGISTER     = 0x00 # Register read
RF24_CMD_W_REGISTER     = 0x20 # Register write
RF24_CMD_ACTIVATE       = 0x50 # (De)Activates R_RX_PL_WID, W_ACK_PAYLOAD, W_TX_PAYLOAD_NOACK features
RF24_CMD_R_RX_PL_WID    = 0x60 # Read RX-payload width for the top R_RX_PAYLOAD in the RX FIFO.
RF24_CMD_R_RX_PAYLOAD   = 0x61 # Read RX payload
RF24_CMD_W_TX_PAYLOAD   = 0xA0 # Write TX payload
RF24_CMD_W_ACK_PAYLOAD  = 0xA8 # Write ACK payload
RF24_CMD_W_TX_PAYLOAD_NOACK = 0xB0
RF24_CMD_FLUSH_TX       = 0xE1 # Flush TX FIFO
RF24_CMD_FLUSH_RX       = 0xE2 # Flush RX FIFO
RF24_CMD_REUSE_TX_PL    = 0xE3 # Reuse TX payload
RF24_CMD_LOCK_UNLOCK    = 0x50 # Lock/unlock exclusive features
RF24_CMD_NOP            = 0xFF # No operation (used for reading status register)

RF24_REG_CONFIG         = 0x00 # Configuration register
RF24_REG_EN_AA          = 0x01 # Enable "Auto acknowledgment"
RF24_REG_EN_RXADDR      = 0x02 # Enable RX addresses
RF24_REG_SETUP_AW       = 0x03 # Setup of address widths
RF24_REG_SETUP_RETR     = 0x04 # Setup of automatic retransmit
RF24_REG_RF_CH          = 0x05 # RF channel
RF24_REG_RF_SETUP       = 0x06 # RF setup register
RF24_REG_STATUS         = 0x07 # Status register
RF24_REG_OBSERVE_TX     = 0x08 # Transmit observe register
RF24_REG_RPD            = 0x09 # Received power detector
RF24_REG_RX_ADDR_P0     = 0x0A # Receive address data pipe 0
RF24_REG_RX_ADDR_P1     = 0x0B # Receive address data pipe 1
RF24_REG_RX_ADDR_P2     = 0x0C # Receive address data pipe 2
RF24_REG_RX_ADDR_P3     = 0x0D # Receive address data pipe 3
RF24_REG_RX_ADDR_P4     = 0x0E # Receive address data pipe 4
RF24_REG_RX_ADDR_P5     = 0x0F # Receive address data pipe 5
RF24_REG_TX_ADDR        = 0x10 # Transmit address
RF24_REG_RX_PW_P0       = 0x11 # Number of bytes in RX payload in data pipe 0
RF24_REG_RX_PW_P1       = 0x12 # Number of bytes in RX payload in data pipe 1
RF24_REG_RX_PW_P2       = 0x13 # Number of bytes in RX payload in data pipe 2
RF24_REG_RX_PW_P3       = 0x14 # Number of bytes in RX payload in data pipe 3
RF24_REG_RX_PW_P4       = 0x15 # Number of bytes in RX payload in data pipe 4
RF24_REG_RX_PW_P5       = 0x16 # Number of bytes in RX payload in data pipe 5
RF24_REG_FIFO_STATUS    = 0x17 # FIFO status register
RF24_REG_DYNPD          = 0x1C # Enable dynamic payload length
RF24_REG_FEATURE        = 0x1D # Feature register

RF24_CONFIG_PRIM_RX         = 0x01
RF24_CONFIG_PWR_UP          = 0x02
RF24_CONFIG_CRCO            = 0x04
RF24_CONFIG_EN_CRC          = 0x08
RF24_CONFIG_MASK_MAX_RT	    = 0x10
RF24_CONFIG_MASK_TX_DS      = 0x20
RF24_CONFIG_MASK_DX_DR      = 0x40

RF24_FEATURE_EN_DYN_ACK   = 0x01 # EN_DYN_ACK bit in FEATURE register
RF24_FEATURE_EN_ACK_PAY   = 0x02 # EN_ACK_PAY bit in FEATURE register
RF24_FEATURE_EN_DPL       = 0x04 # EN_DPL bit in FEATURE register
RF24_FLAG_RX_DR           = 0x40 # RX_DR bit (data ready RX FIFO interrupt)
RF24_FLAG_TX_DS           = 0x20 # TX_DS bit (data sent TX FIFO interrupt)
RF24_FLAG_MAX_RT          = 0x10 # MAX_RT bit (maximum number of TX retransmits interrupt)

RF24_MASK_REG_MAP         = 0x1F # Mask bits[4:0] for CMD_RREG and CMD_WREG commands
RF24_MASK_CRC             = 0x0C # Mask for CRC bits [3:2] in CONFIG register
RF24_MASK_STATUS_IRQ      = 0x70 # Mask for all IRQ bits in STATUS register
RF24_MASK_RF_PWR          = 0x06 # Mask RF_PWR[2:1] bits in RF_SETUP register
RF24_MASK_RX_P_NO         = 0x0E # Mask RX_P_NO[3:1] bits in STATUS register
RF24_MASK_DATARATE        = 0x28 # Mask RD_DR_[5,3] bits in RF_SETUP register
RF24_MASK_EN_RX           = 0x3F # Mask ERX_P[5:0] bits in EN_RXADDR register
RF24_MASK_RX_PW           = 0x3F # Mask [5:0] bits in RX_PW_Px register
RF24_MASK_RETR_ARD        = 0xF0 # Mask for ARD[7:4] bits in SETUP_RETR register
RF24_MASK_RETR_ARC        = 0x0F # Mask for ARC[3:0] bits in SETUP_RETR register
RF24_MASK_RXFIFO          = 0x03 # Mask for RX FIFO status bits [1:0] in FIFO_STATUS register
RF24_MASK_TXFIFO          = 0x30 # Mask for TX FIFO status bits [5:4] in FIFO_STATUS register
RF24_MASK_PLOS_CNT        = 0xF0 # Mask for PLOS_CNT[7:4] bits in OBSERVE_TX register
RF24_MASK_ARC_CNT         = 0x0F # Mask for ARC_CNT[3:0] bits in OBSERVE_TX register

class RF24:
    def __init__(self, board :Board, ce: int, csn: int):
        self.board = board
        self.ce = ce
        self.csn = csn

    def CE_H(self):
        self.board.write_pin(self.ce, True)

    def CE_L(self):
        self.board.write_pin(self.ce, False)

    def CSN_H(self):
        self.board.write_pin(self.csn, True)

    def CSN_L(self):
        self.board.write_pin(self.csn, False)

    def read_reg(self, reg):
        self.CSN_L()
        self.board.spi_rw(reg)
        value = self.board.spi_rw(RF24_CMD_NOP)
        self.CSN_H()

        return value

    def write_reg(self, reg, value):
        self.CSN_L()
        if reg < RF24_CMD_W_REGISTER:
            self.board.spi_rw(RF24_CMD_W_REGISTER | reg)
            self.board.spi_rw(value)
        else:
            self.board.spi_rw(reg)
            if not reg in (RF24_CMD_FLUSH_RX,
                           RF24_CMD_FLUSH_TX,
                           RF24_CMD_REUSE_TX_PL,
                           RF24_CMD_NOP):
                self.board.spi_rw(value)

        self.CSN_H()

    def write_buf(self, reg, data):
        self.CSN_L()
        self.board.spi_rw(reg)
        for byte in data:
            self.board.spi_rw(byte)
        self.CSN_H()

    def read_buf(self, reg, len):
        buf = []
        self.CSN_L()
        self.board.spi_rw(reg)
        for _ in range(len):
            val = self.board.spi_rw(RF24_CMD_NOP)
            buf.append(val)
        self.CSN_H()

        return buf

    def read_buf_as_bytes(self, reg, len):
        return struct.pack("B" * len, *self.read_buf(reg, len))

    def is_connected(self):
        test_addr = b"nrf24"
        self.write_buf(RF24_CMD_W_REGISTER | RF24_REG_TX_ADDR, test_addr)
        self.write_buf(RF24_CMD_W_REGISTER | RF24_REG_TX_ADDR, test_addr)
        result = self.read_buf_as_bytes(RF24_REG_TX_ADDR, 5)
        return test_addr == result

    def clear_irq_flags(self):
        reg = self.read_reg(RF24_REG_STATUS)
        reg |= RF24_MASK_STATUS_IRQ
        self.write_reg(RF24_REG_STATUS ,reg)

    def set_tx_addr(self, addr):
        self.write_buf(RF24_CMD_W_REGISTER | RF24_REG_TX_ADDR, addr)

    def get_tx_addr(self):
        return self.read_buf(RF24_REG_TX_ADDR, 5)

    _rx_addr = [
        RF24_REG_RX_ADDR_P0,
        RF24_REG_RX_ADDR_P1,
        RF24_REG_RX_ADDR_P2,
        RF24_REG_RX_ADDR_P3,
        RF24_REG_RX_ADDR_P4,
        RF24_REG_RX_ADDR_P5,
    ]

    def set_rx_addr(self, pipe, addr):
        self.write_buf(RF24_CMD_W_REGISTER | RF24._rx_addr[pipe], addr)

    def get_rx_addr(self, pipe):
        if pipe == 0 or pipe == 1:
            length = 5
        else:
            length = 2
        return self.read_buf(RF24._rx_addr[pipe], length)

    def flush_rx(self):
        self.write_reg(RF24_CMD_FLUSH_RX, 0x00)

    def flush_tx(self):
        self.write_reg(RF24_CMD_FLUSH_TX, 0x00)

def start_lisening (rf):
    if not rf.is_connected():
        raise Exception("Device is not connected")

    rf.CE_L()

    #rf.write_reg(RF24_REG_EN_AA, 0x00) # disable AA
    rf.write_reg(RF24_REG_EN_AA, 0x01) # enable AA on pipe 0
    rf.write_reg(RF24_REG_EN_RXADDR, 0x01) # enable pipe 0
    rf.write_reg(RF24_REG_SETUP_AW, 0b11) # 5 bytes address
    rf.write_reg(RF24_REG_SETUP_RETR, 0x00) # diable retry
    rf.write_reg(RF24_REG_RF_CH, 40) # channel 40
    rf.write_reg(RF24_REG_RF_SETUP, 0b00001011) # 2Mbps 0dbm
    #rf.set_rx_addr(0, [0x34, 0x43, 0x10, 0x10, 0x01])
    rf.set_rx_addr(0, b'ziyan')
    #rf.set_tx_addr([0x34, 0x43, 0x10, 0x10, 0x01])
    rf.set_tx_addr(b'ziyan')
    #rf.write_reg(RF24_REG_RX_PW_P0, 22)
    rf.write_reg(RF24_REG_RX_PW_P0, 1)

    # RX
    rf.write_reg(RF24_REG_CONFIG,
        0b00001111)

    rf.flush_rx()
    rf.flush_tx()

    rf.clear_irq_flags()

    rf.CE_H()

def start_sending (rf):
    if not rf.is_connected():
        raise Exception("Device is not connected")

    rf.CE_L()

    #rf.write_reg(RF24_REG_EN_AA, 0x00) # disable AA
    rf.write_reg(RF24_REG_EN_AA, 0x01) # enable AA on pipe 0
    rf.write_reg(RF24_REG_EN_RXADDR, 0x01) # enable pipe 0
    rf.write_reg(RF24_REG_SETUP_AW, 0b11) # 5 bytes address
    rf.write_reg(RF24_REG_SETUP_RETR, 0x00) # diable retry
    rf.write_reg(RF24_REG_RF_CH, 40) # channel 40
    rf.write_reg(RF24_REG_RF_SETUP, 0b00001011) # 2Mbps 0dbm
    #rf.write_reg(RF24_REG_RF_SETUP, 0b00100011) # 250kbps
    #rf.set_rx_addr(0, [0x34, 0x43, 0x10, 0x10, 0x01])
    rf.set_rx_addr(0, b'ziyan')
    #rf.set_tx_addr([0x34, 0x43, 0x10, 0x10, 0x01])
    rf.set_tx_addr(b'ziyan')
    rf.write_reg(RF24_REG_RX_PW_P0, 1)
    #rf.write_reg(RF24_REG_RX_PW_P0, 1)    # enable tx
    rf.write_reg(RF24_REG_CONFIG,
        0b00001110)

    rf.flush_rx()
    rf.flush_tx()

    rf.clear_irq_flags()

    rf.CE_H()


if __name__ == '__main__':
    from time import sleep
    board1 = Board(serial.Serial('COM11'))
    #board2 = Board(serial.Serial('COM11'))

    board1.write_pin(PIN_LED, 1)
    #board2.write_pin(PIN_LED, 1)
    rx = RF24(board1, PIN_CE, PIN_CSN)
    tx = RF24(board1, PIN_CE, PIN_CSN)
    start_lisening(rx)
    #start_sending(tx)
    LED = True
    while True:
        board1.write_pin(PIN_LED, LED)
        #board2.write_pin(PIN_LED, not LED)
        LED = not LED

        #tx.CE_L()
        #tx.flush_tx()
        #tx.clear_irq_flags()
        #tx.write_buf(RF24_CMD_W_TX_PAYLOAD, [0x05] + 16 * [0xff])
        #sleep(0.01)
        #tx.CE_H()

        #print(f"{tx.read_reg(RF24_REG_STATUS):08b}")

        #print("".join(map(chr, rx.get_rx_addr(0))))
        if (rx.board.read_pin(PIN_IRQ) == False):
            rx.CE_L()
            pld = rx.read_buf_as_bytes(RF24_CMD_R_RX_PAYLOAD, 1)
            print(f"received: {pld}")
            rx.clear_irq_flags()
            rx.CE_H()
            
        #rx.CE_L()
        #rx.flush_tx()
        #rx.clear_irq_flags()
        #rx.write_buf(RF24_CMD_W_TX_PAYLOAD, [0] + [0xFF] * 16)
        #sleep(0.01)
        #rx.CE_H()

        sleep(0.1)
