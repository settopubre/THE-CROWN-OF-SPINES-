package main

import (
	"encoding/json"
	"fmt"
	"strings"

	tea "github.com/charmbracelet/bubbletea"
)

type Msg struct {
	Role    string
	Content string
}

type model struct {
	input      string
	messages   []Msg
	ollama     *OllamaClient
	bridge     *EngineBridge
	thinking   bool
	config     map[string]interface{}
	width      int
	height     int
	streamChan chan string
}

type initEngineMsg struct{}
type ollamaChunkMsg struct {
	text string
	done bool
}
type errorMsg struct{ err error }

func initialModel(cfg map[string]interface{}) tea.Model {
	socket, _ := cfg["socket_path"].(string)
	url, _ := cfg["ollama_endpoint"].(string)
	mod, _ := cfg["ollama_model"].(string)
	if socket == "" {
		socket = DefaultSocketPath
	}
	if url == "" {
		url = DefaultOllamaURL
	}
	if mod == "" {
		mod = DefaultModel
	}

	return &model{
		config:     cfg,
		ollama:     &OllamaClient{Endpoint: url, Model: mod},
		messages:   []Msg{{Role: "system", Content: "3DAI Oracle Ready. /engine <cmd> or type query."}},
		streamChan: make(chan string, 64),
	}
}

func waitForStream(ch <-chan string) tea.Cmd {
	return func() tea.Msg {
		txt, ok := <-ch
		if !ok {
			return ollamaChunkMsg{"", true}
		}
		return ollamaChunkMsg{txt, txt == "\n[DONE]"}
	}
}

func (m *model) Init() tea.Cmd {
	return tea.Batch(
		func() tea.Msg {
			bridge, err := NewEngineBridge(m.config["socket_path"].(string))
			if err != nil {
				return errorMsg{err}
			}
			m.bridge = bridge
			return initEngineMsg{}
		},
		waitForStream(m.streamChan),
	)
}

func (m *model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.Type {
		case tea.KeyEnter:
			if m.input == "" {
				return m, nil
			}
			m.messages = append(m.messages, Msg{Role: "user", Content: m.input})
			return m, m.dispatch(m.input)
		case tea.KeyBackspace:
			if len(m.input) > 0 {
				m.input = m.input[:len(m.input)-1]
			}
		case tea.KeyCtrlC, tea.KeyEsc:
			return m, tea.Quit
		case tea.KeyRunes:
			m.input += string(msg.Runes)
		}
	case initEngineMsg:
		m.messages = append(m.messages, Msg{Role: "system", Content: "✅ Engine Connected"})
	case ollamaChunkMsg:
		if msg.done {
			m.thinking = false
			return m, nil
		}
		if len(m.messages) > 0 && m.messages[len(m.messages)-1].Role == "ai" {
			m.messages[len(m.messages)-1].Content += msg.text
		} else {
			m.messages = append(m.messages, Msg{Role: "ai", Content: msg.text})
		}
		return m, nil
	case errorMsg:
		m.messages = append(m.messages, Msg{Role: "error", Content: msg.err.Error()})
	case tea.WindowSizeMsg:
		m.width, m.height = msg.Width, msg.Height
	}
	return m, nil
}

func (m *model) dispatch(cmd string) tea.Cmd {
	m.thinking = true
	m.input = ""
	if strings.HasPrefix(cmd, "/engine ") {
		payload := make(BridgePayload)
		parts := strings.Fields(strings.TrimPrefix(cmd, "/engine "))
		if len(parts) > 0 {
			payload["command"] = parts[0]
		}
		if len(parts) > 1 {
			payload["args"] = strings.Join(parts[1:], " ")
		}
		return func() tea.Msg {
			resp, err := m.bridge.Send("execute", payload)
			if err != nil {
				return errorMsg{err}
			}
			b, _ := json.Marshal(resp)
			return Msg{Role: "engine", Content: string(b)}
		}
	}

	go m.ollama.StreamPrompt(fmt.Sprintf("%s\nQuery: %s", PromptPrefix, cmd), m.streamChan)
	return nil
}
