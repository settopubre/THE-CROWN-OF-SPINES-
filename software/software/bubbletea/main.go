package main

import (
	"encoding/json"
	"log"
	"os"

	tea "github.com/charmbracelet/bubbletea"
)

func main() {
	cfg := map[string]interface{}{}
	if data, err := os.ReadFile("config.json"); err == nil {
		json.Unmarshal(data, &cfg)
	}

	m := initialModel(cfg)
	p := tea.NewProgram(m)
	if _, err := p.Run(); err != nil {
		log.Fatal(err)
	}
}
