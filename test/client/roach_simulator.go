package main
 
import (
    "fmt"
    "net"
    "os"
    "time"
    "strconv"
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
    i := 0
    for {
        msg := strconv.Itoa(i)
        i++
        buf := []byte(msg)
        _,err := Conn.Write(buf)
        if err != nil {
            fmt.Println(msg, err)
        }
        time.Sleep(time.Second * 1)
    }
}
