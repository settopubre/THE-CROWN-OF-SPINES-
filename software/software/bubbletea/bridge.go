package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net"
	"sync"
)

type BridgePayload map[string]interface{}

type BridgeMessage struct {
	Action string        `json:"action"`
	Data   BridgePayload `json:"payload,omitempty"`
}

type BridgeResponse struct {
	Status  string      `json:"status"`
	Data    interface{} `json:"data,omitempty"`
	Message string      `json:"message,omitempty"`
}

type EngineBridge struct {
	conn net.Conn
	mux  sync.Mutex
}

func NewEngineBridge(socketPath string) (*EngineBridge, error) {
	conn, err := net.Dial("unix", socketPath)
	if err != nil {
		return nil, fmt.Errorf("bridge connect: %w", err)
	}
	return &EngineBridge{conn: conn}, nil
}

func (b *EngineBridge) Close() {
	if b.conn != nil {
		b.conn.Close()
	}
}

func (b *EngineBridge) Send(cmd string, payload BridgePayload) (BridgeResponse, error) {
	b.mux.Lock()
	defer b.mux.Unlock()

	msg := BridgeMessage{Action: cmd, Data: payload}
	data, _ := json.Marshal(msg)
	data = append(data, '\n')

	_, err := b.conn.Write(data)
	if err != nil {
		return BridgeResponse{}, err
	}

	reader := bufio.NewReader(b.conn)
	line, err := reader.ReadString('\n')
	if err != nil {
		return BridgeResponse{}, err
	}

	var resp BridgeResponse
	err = json.Unmarshal([]byte(line), &resp)
	return resp, err
}
