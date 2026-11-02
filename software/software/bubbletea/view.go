package main

import (
	"fmt"
	"strings"
)

func (m *model) View() string {
	var b strings.Builder
	b.WriteString(TitleStyle.Render("3DAI CONCRETE ORACLE\n"))
	for _, msg := range m.messages {
		s := BaseStyle
		switch msg.Role {
		case "user":
			s = UserMsgStyle
		case "ai":
			s = AISystemStyle
		case "engine":
			s = EngineRespStyle
		case "error":
			s = ErrorStyle
		default:
			s = StatusStyle
		}
		b.WriteString(s.Render(fmt.Sprintf("[%s]: %s\n", strings.ToUpper(msg.Role), msg.Content)))
	}
	if m.thinking {
		b.WriteString(StatusStyle.Render("🤔 Oracle thinking...\n"))
	}
	b.WriteString(PromptStyle.Render("> " + m.input + "█"))
	return BaseStyle.Width(m.width).Height(m.height - 2).Render(b.String())
}
