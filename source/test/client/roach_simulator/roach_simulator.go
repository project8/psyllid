package main
 
import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"math"
	"net"
	"os"
	"strconv"
	"time"
)
 
func CheckError(err error) {
	if err  != nil {
		fmt.Println("Error: " , err)
		os.Exit(1)
	}
}

const PayloadSize int16 = 8192

type RawPacket struct {
	word0, word1, word2, word3 uint64
	payload [PayloadSize]int8
}

type RoachPacket struct {
	// --- word 0 --
	// unixTime = 32 bits
	// pktInBatch = 20 bits
	// digitalID = 6 bits
	// ifID = 6 bits
	// --- word 1 ---
	// userData1 = 32 bits
	// userData2 = 32 bits
	// --- word 2 ---
	// reserved0 = 64 bits
	// --- word 3 ---
	// reserved1 = 63 bits
	// freqNotTime = 1 bit
	unixTime, pktInBatch, digitalID, ifID, userData1, userData0 uint32
	reserved0, reserved1 uint64
	freqNotTime uint8
	payload [PayloadSize]int8
}

func (roachPkt *RoachPacket)PackInto( rawPkt *RawPacket ) {
	(*rawPkt).word0 = uint64((*roachPkt).unixTime) |
						(uint64((*roachPkt).pktInBatch) << 32) |
						(uint64((*roachPkt).digitalID) << 52) |
						(uint64((*roachPkt).ifID) << 58)

	(*rawPkt).word1 = uint64((*roachPkt).userData1) | 
						(uint64((*roachPkt).userData1) << 32)

	(*rawPkt).word2 = (*roachPkt).reserved0

	(*rawPkt).word3 = (*roachPkt).reserved1 |
						(uint64((*roachPkt).freqNotTime) << 63)

	rawPayloadSlice := (*rawPkt).payload[:]
	roachPayloadSlice := (*roachPkt).payload[:]
	copy(rawPayloadSlice, roachPayloadSlice)

	return
}
 
func main() {
	// Notes on the timing between packets
	// Packets are sent in (time, freqquency) pairs.  There is a 1 ms delay between them (configurable with the -tf-delay flag)
	// The delay time between pairs is configurable with the -pkt-delay command-line flag.  The default is 500 ms.
	// For delays much larger than 1 ms, this is approximately the period of the packet cycle.

	// setup command line options
	var ip string
	var port int
	var packetDelay, tfDelay time.Duration
	flag.StringVar(&ip, "ip", "127.0.0.1", "IP address to which the packets are sent")
	flag.IntVar(&port, "port", 23530, "Port to which the packets are sent")
	flag.DurationVar(&packetDelay, "pkt-delay", time.Millisecond*500, "Delay time between packets (approximately the period of the packet cycle)")
	flag.DurationVar(&tfDelay, "tf-delay", time.Millisecond, "Delay between the time and frequency packets")

	flag.Parse()

	fmt.Printf("Time between packet pairs: %v\n", packetDelay)
	fmt.Printf("Time between time and frequency packets: %v\n", tfDelay)

	address := ip + ":" + strconv.Itoa(port)
	fmt.Printf("Addressing packets to: %v\n", address)
	//ServerAddr,err := net.ResolveUDPAddr("udp","127.0.0.1:23530")
	ServerAddr,err := net.ResolveUDPAddr("udp", address)
	CheckError(err)
 
	//LocalAddr, err := net.ResolveUDPAddr("udp", "127.0.0.1:0")
	//CheckError(err)
 
	Conn, err := net.DialUDP("udp", nil, ServerAddr)
	CheckError(err)
	defer Conn.Close()

	var rawPkt RawPacket
	timePkt := RoachPacket{
		digitalID: 0,
		ifID: 0,
		freqNotTime: 0,
	}
	freqPkt := RoachPacket{
		digitalID: 0,
		ifID: 0,
		freqNotTime: 1,
	}

	// fill the payload
	for i := 0; i < int(PayloadSize); i+=2 {
		timePkt.payload[i] = int8(256.0 * math.Sin( float64(i) * math.Pi / 16.0 ))
		timePkt.payload[i+1] = int8(5)
		freqPkt.payload[i] = int8(1)
		freqPkt.payload[i+1] = int8(2)
	}

	var packetCounter uint32 = 0
	const packetCounterMax uint32 = 390625

	buffer := new( bytes.Buffer )

	var signalCounter uint8 = 0
	const signalCounterMax uint8 = 15
	const signalTrigger uint8 = 14

	fmt.Println()
		
	for {

		for i := 0; i < 2; i++ {
			// convert the appropriate roach packet to rawPkt
			if i == 0 {
				timePkt.unixTime = uint32(time.Now().Unix())
				timePkt.pktInBatch = packetCounter
				(&timePkt).PackInto( &rawPkt )
				fmt.Printf( "Sending (%v bytes): time = %v,  id = %v,  freqNotTime = %v\n", buffer.Len(), timePkt.unixTime, timePkt.pktInBatch, timePkt.freqNotTime )
			} else {
				freqPkt.unixTime = uint32(time.Now().Unix())
				freqPkt.pktInBatch = packetCounter
				if signalCounter == signalTrigger {
					freqPkt.payload[3] = 100
					fmt.Printf( "Trigger!\n" )
				} else {
					freqPkt.payload[3] = 1
				}
				(&freqPkt).PackInto( &rawPkt )
				fmt.Printf( "Sending (%v bytes): time = %v,  id = %v,  freqNotTime = %v\n", buffer.Len(), freqPkt.unixTime, freqPkt.pktInBatch, freqPkt.freqNotTime )
			}

			fmt.Printf( "Raw packet header: %x, %x, %x, %x\n", rawPkt.word0, rawPkt.word1, rawPkt.word2, rawPkt.word3 )

			// write to the bytes buffer
			buffer.Reset()
			bytesErr := binary.Write( buffer, binary.BigEndian, rawPkt )
			CheckError(bytesErr)

			// send over the UDP connection
			_, err := Conn.Write(buffer.Bytes())
			if err != nil {
				fmt.Println("Unable to send buffer:", err)
			}
			time.Sleep(tfDelay)
		}
		time.Sleep(packetDelay)

		packetCounter++
		if packetCounter == packetCounterMax {
			packetCounter = 0
		}

		signalCounter++
		if signalCounter == signalCounterMax {
			signalCounter = 0
		}

	}
}
