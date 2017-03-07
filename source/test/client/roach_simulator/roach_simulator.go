package main

/*
Usage:
> roach_simulator [options]

Options:
-ip (string) - specify the IP address to which the packets are sent
-port (string) - specify the port to which the packets are sent
-pkt-delay (duration) - specify the time between time/frequency packet pairs (in time.ParseDuration form; e.g. 500ms, 1s, etc)
-tf-delay (duration)- specify the time between the time and frequency packets in each packet-pair (in time.ParseDuration form)
-do-trig (bool) - flag to specify whether to do the trigger in the frequency packets
-incl-pkt-num (bool) - flag to specify whether to include the packet number as the first bin in the time and frequency packets

Notes on the timing between packets:
Packets are sent in (time, frequency) pairs.  There is a 1 ms delay between them (configurable with the -tf-delay flag)
The delay time between pairs is configurable with the -pkt-delay command-line flag.  The default is 500 ms.
For delays much larger than 1 ms, this is approximately the period of the packet cycle.
*/

 
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
	payload [PayloadSize]byte
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
	payload []byte
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
	// setup command line options
	var ip string
	var port int
	var packetDelay, tfDelay time.Duration
	var doTrigger, includePktNum bool
	flag.StringVar(&ip, "ip", "127.0.0.1", "IP address to which the packets are sent")
	flag.IntVar(&port, "port", 23530, "Port to which the packets are sent")
	flag.DurationVar(&packetDelay, "pkt-delay", time.Millisecond*500, "Delay time between packets (approximately the period of the packet cycle)")
	flag.DurationVar(&tfDelay, "tf-delay", time.Millisecond, "Delay between the time and frequency packets")
	flag.BoolVar(&doTrigger, "do-trig", true, "Flag for whether to do the trigger in the frequency packets")
	flag.BoolVar(&includePktNum, "incl-pkt-num", true, "Flag for whether to include the packet number in the first bin of the time and frequency packets")

	flag.Parse()

	fmt.Printf("Time between packet pairs: %v\n", packetDelay)
	fmt.Printf("Time between time and frequency packets: %v\n", tfDelay)

	address := ip + ":" + strconv.Itoa(port)
	fmt.Printf("Addressing packets to: %v\n", address)
	//ServerAddr,err := net.ResolveUDPAddr("udp","127.0.0.1:23530")
	ServerAddr,err := net.ResolveUDPAddr("udp", address)
	CheckError(err)
 
	Conn, err := net.DialUDP("udp", nil, ServerAddr)
	CheckError(err)
	defer Conn.Close()

	var rawPkt RawPacket
	timePkt := RoachPacket{
		digitalID: 0,
		ifID: 0,
		freqNotTime: 0,
	}
	timePkt.payload = make([]byte, PayloadSize)
	freqPkt := RoachPacket{
		digitalID: 0,
		ifID: 0,
		freqNotTime: 1,
	}
	freqPkt.payload = make([]byte, PayloadSize)
	fmt.Printf("Payload size: %v\n", PayloadSize)
	
	// fill the slices that we'll use to write to the payloads
    timePayloadBuf := new(bytes.Buffer)
    freqPayloadBuf := new(bytes.Buffer)
    var value uint8
	timePayloadFirst8 := make([]int8, 8) // true order
	freqPayloadFirst8 := make([]int8, 8) // true order
    var truePayloadIndex int
	for iWord := 0; iWord < int(PayloadSize/8); iWord++ {
	    for iByte := 6; iByte >=0; iByte-=2 {
            truePayloadIndex = iWord*8 + iByte
            value = uint8(128.0 * math.Sin( float64(truePayloadIndex/2) * math.Pi / 16.0 ))
            binary.Write(timePayloadBuf, binary.LittleEndian, value)
            value = uint8(5)
            binary.Write(timePayloadBuf, binary.LittleEndian, value)
            value = uint8(1)
            binary.Write(freqPayloadBuf, binary.LittleEndian, value)
            value = uint8(2)
            binary.Write(freqPayloadBuf, binary.LittleEndian, value)
            if iWord == 1 {
                //fmt.Printf( "%v ", timePayload64Bit[iWord])
                timePayloadFirst8[iByte] = int8(128.0 * math.Sin( float64(truePayloadIndex/2) * math.Pi / 16.0 ))
                timePayloadFirst8[iByte+1] = 5
                freqPayloadFirst8[iByte] = 1
                freqPayloadFirst8[iByte+1] = 2
            }
	    }
	    //if iWord < 10 {
        //    fmt.Printf( "\n$$$ 8 bytes: %v\n", timePayloadFirst8)
	    //}
	}
	
    // copy write buffers to payloads
    copy(timePkt.payload, timePayloadBuf.Bytes())
	copy(freqPkt.payload, freqPayloadBuf.Bytes())
	
    fmt.Printf( "First 8 elements of the time and freq payloads in true order as signed integers\n(some elements may be changed while simulating each packet)\n" )
    fmt.Printf( "Time:%v\n", timePayloadFirst8 )
    fmt.Printf( "Freq:%v\n", freqPayloadFirst8 )
    fmt.Printf( "First 8 elements of the time and freq payloads in as-sent order as unsigned integers\n(some elements may be changed while simulating each packet)\n" )
    fmt.Printf( "Time:%v\n", timePkt.payload[:8] )
    fmt.Printf( "Freq:%v\n", freqPkt.payload[:8] )

	var packetCounter uint32 = 0
	const packetCounterMax uint32 = 390625

	buffer := new( bytes.Buffer )

    var bin0Counter byte = 0
	var signalCounter byte = 0
	const signalCounterMax uint8 = 15
	const signalTrigger uint8 = 14

	fmt.Println()
		
	for {
	    if includePktNum {
            timePkt.payload[6] = bin0Counter  // put in bin 6 because of byte reordering
            freqPkt.payload[6] = bin0Counter  // put in bin 6 because of byte reordering
        }
		for i := 0; i < 2; i++ {
			// convert the appropriate roach packet to rawPkt
			if i == 0 {
				timePkt.unixTime = uint32(time.Now().Unix())
				timePkt.pktInBatch = packetCounter
				(&timePkt).PackInto( &rawPkt )
				fmt.Printf( "Sending (%v bytes): time = %v,  id = %v,  freqNotTime = %v\n", buffer.Len(), timePkt.unixTime, timePkt.pktInBatch, timePkt.freqNotTime )
				fmt.Printf( "Payload (8 elements, reverse of true order): %v\n", timePkt.payload[0:8] )
			} else {
				freqPkt.unixTime = uint32(time.Now().Unix())
				freqPkt.pktInBatch = packetCounter
				if doTrigger {
    				if signalCounter == signalTrigger {
    					freqPkt.payload[3] = 100
    					fmt.Printf( "Trigger!\n" )
    				} else {
    					freqPkt.payload[3] = 1
    				}
				}
				(&freqPkt).PackInto( &rawPkt )
				fmt.Printf( "Sending (%v bytes): time = %v,  id = %v,  freqNotTime = %v\n", buffer.Len(), freqPkt.unixTime, freqPkt.pktInBatch, freqPkt.freqNotTime )
				fmt.Printf( "Payload (first 8 elements, reverse of true order): %v\n", freqPkt.payload[0:8] )
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
		
		bin0Counter++
	}
	
	return
}
