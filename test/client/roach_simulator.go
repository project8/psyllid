package main
 
import (
	"bytes"
	"encoding/binary"
	"fmt"
	"math"
	"net"
	"os"
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
	reserved0, reserved1, freqNotTime uint64
	payload [PayloadSize]int8
}

func (roachPkt *RoachPacket)PackInto( rawPkt *RawPacket ) {
	(*rawPkt).word0 = uint64((*roachPkt).unixTime) |
						(uint64((*roachPkt).pktInBatch) >> 32) |
						(uint64((*roachPkt).digitalID) >> 52) |
						(uint64((*roachPkt).ifID) >> 58)

	(*rawPkt).word1 = uint64((*roachPkt).userData1) | 
						(uint64((*roachPkt).userData1) >> 32)

	(*rawPkt).word2 = (*roachPkt).reserved0

	(*rawPkt).word3 = (*roachPkt).reserved1 |
						((*roachPkt).freqNotTime >> 63)

	rawPayloadSlice := (*rawPkt).payload[:]
	roachPayloadSlice := (*roachPkt).payload[:]
	copy(rawPayloadSlice, roachPayloadSlice)

	return
}
 
func main() {
	ServerAddr,err := net.ResolveUDPAddr("udp","127.0.0.1:23530")
	CheckError(err)
 
	//LocalAddr, err := net.ResolveUDPAddr("udp", "127.0.0.1:0")
	//CheckError(err)
 
	Conn, err := net.DialUDP("udp", nil, ServerAddr)
	CheckError(err)
	defer Conn.Close()

	var rawPkt RawPacket
	roachPkt := RoachPacket{
		digitalID: 0,
		ifID: 0,
		freqNotTime: 0,
	}

	// fill the payload
	for i := 0; i < int(PayloadSize); i++ {
		roachPkt.payload[i] = int8(256.0 * math.Sin( float64(i) * math.Pi / 16.0 ))
	}

	var packetCounter uint32 = 0;
	const packetCounterMax uint32 = 390625;

	for {
		buffer := new( bytes.Buffer )
		
		// fill roachPkt
		roachPkt.unixTime = uint32(time.Now().Unix())
		roachPkt.pktInBatch = packetCounter

		// convert roachPkt to rawPkt
		(&roachPkt).PackInto( &rawPkt )

		// write to the bytes buffer
		bytesErr := binary.Write( buffer, binary.BigEndian, rawPkt )
		CheckError(bytesErr)

		fmt.Printf( "Sending: time = %v,  id = %v\n", roachPkt.unixTime, roachPkt.pktInBatch )

		// send over the UDP connection
		_,err := Conn.Write(buffer.Bytes())
		if err != nil {
			fmt.Println("Unable to send buffer:", err)
		}
		time.Sleep(time.Second * 1)

		packetCounter++
		if packetCounter == packetCounterMax {
			packetCounter = 0
		}

	}
}
