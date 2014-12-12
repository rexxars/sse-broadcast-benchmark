package main

import (
    "os"
    "fmt"
    "log"
    "net/http"
    "time"
    "strconv"
)

const (
    msgBuf = 50
)

type SSE struct{}

func (s *SSE) ServeHTTP(rw http.ResponseWriter, req *http.Request) {
    f, ok := rw.(http.Flusher)
    if !ok {
        http.Error(rw, "cannot stream", http.StatusInternalServerError)
        return
    }

    if req.Method == "OPTIONS" {
        rw.Header().Set("Access-Control-Allow-Origin", "*")
        rw.Header().Set("Connection", "close")
        rw.WriteHeader(204)
        return
    }

    rw.Header().Set("Content-Type", "text/event-stream")
    rw.Header().Set("Cache-Control", "no-cache")
    rw.Header().Set("Access-Control-Allow-Origin", "*")
    rw.Header().Set("Connection", "keep-alive")
    fmt.Fprintf(rw, ":ok\n\n")
    f.Flush()

    cn, ok := rw.(http.CloseNotifier)
    if !ok {
        http.Error(rw, "cannot stream", http.StatusInternalServerError)
        return
    }

    messages := msgBroker.Subscribe()

    for {
        select {
        case <-cn.CloseNotify():
            msgBroker.Unsubscribe(messages)
            return
        case msg := <-messages:
            fmt.Fprintf(rw, "data: %s\n\n", msg)
            f.Flush()
        }
    }
}

func main() {
    msgBroker = NewBroker()

    port := "1942"
    if len(os.Args) > 2 {
        port = os.Args[2]
    }

    http.Handle("/sse", &SSE{})
    http.HandleFunc("/connections", func(w http.ResponseWriter, req *http.Request) {
        w.Header().Set("Content-Type", "text/plain")
        w.Header().Set("Cache-Control", "no-cache")
        w.Header().Set("Access-Control-Allow-Origin", "*")
        w.Header().Set("Connection", "close")
        fmt.Fprintf(w, strconv.Itoa(msgBroker.SubscriberCount()))
    })

    go func() {
        for {
            msg := strconv.FormatInt(time.Now().UnixNano() / 1000000, 10);
            msgBroker.Publish([]byte(msg))
            time.Sleep(time.Second)
        }
    }()

    fmt.Println("Listening on http://127.0.0.1:" + port + "/")
    log.Fatal(http.ListenAndServe(":" + port, nil))
}

type Broker struct {
    subscribers map[chan []byte]bool
}

func (b *Broker) Subscribe() chan []byte {
    ch := make(chan []byte, msgBuf)
    b.subscribers[ch] = true
    return ch
}

func (b *Broker) Unsubscribe(ch chan []byte) {
    delete(b.subscribers, ch)
}

func (b *Broker) SubscriberCount() int {
    return len(b.subscribers)
}

func (b *Broker) Publish(msg []byte) {
    for ch := range b.subscribers {
        ch <- msg
    }
}

func NewBroker() *Broker {
    return &Broker{make(map[chan []byte]bool)}
}

var msgBroker *Broker