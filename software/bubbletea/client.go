package main

import (
	"bufio"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
)

type OllamaRequest struct {
	Model  string `json:"model"`
	Prompt string `json:"prompt"`
	Stream bool   `json:"stream"`
}

type OllamaStreamResponse struct {
	Response string `json:"response"`
	Done     bool   `json:"done"`
}

type OllamaClient struct {
	Endpoint string
	Model    string
}

func (c *OllamaClient) StreamPrompt(prompt string, out chan<- string) error {
	defer close(out)
	reqBody := OllamaRequest{Model: c.Model, Prompt: prompt, Stream: true}
	jsonData, _ := json.Marshal(reqBody)

	resp, err := http.Post(c.Endpoint+"/api/generate", "application/json", bytes.NewBuffer(jsonData))
	if err != nil {
		out <- fmt.Sprintf("Ollama error: %v", err)
		return err
	}
	defer resp.Body.Close()

	reader := bufio.NewReader(resp.Body)
	for {
		line, err := reader.ReadBytes('\n')
		if err != nil && err != io.EOF {
			out <- fmt.Sprintf("Stream error: %v", err)
			return err
		}
		if len(line) == 0 && err == io.EOF {
			break
		}
		var sResp OllamaStreamResponse
		if err := json.Unmarshal(line, &sResp); err == nil {
			if sResp.Response != "" {
				out <- sResp.Response
			}
			if sResp.Done {
				out <- "\n[DONE]"
				break
			}
		}
		if err == io.EOF {
			break
		}
	}
	return nil
}
