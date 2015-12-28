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
 
func main() {
    ServerAddr,err := net.ResolveUDPAddr("udp","127.0.0.1:23530")
    CheckError(err)
 
    //LocalAddr, err := net.ResolveUDPAddr("udp", "127.0.0.1:0")
    //CheckError(err)
 
    Conn, err := net.DialUDP("udp", nil, ServerAddr)
    CheckError(err)
    defer Conn.Close()

    var dig int8 = 0
    for {
    	buffer := new( bytes.Buffer )
    	
    	var fft float64 = math.Sin( float64(dig) * math.Pi / 16.0 )

    	bytesErr := binary.Write( buffer, binary.LittleEndian, dig )
    	CheckError(bytesErr)

    	bytesErr = binary.Write( buffer, binary.LittleEndian, fft )
    	CheckError(bytesErr)

    	fmt.Printf( "Sending: %v --> %v, % x\n", dig, fft, buffer )

        _,err := Conn.Write(buffer.Bytes())
        if err != nil {
            fmt.Println("Unable to send buffer:", err)
        }
        time.Sleep(time.Second * 1)

        dig++
    }
}
