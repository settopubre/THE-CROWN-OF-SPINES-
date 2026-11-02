package main

import "github.com/charmbracelet/lipgloss"

var (
	BaseStyle       = lipgloss.NewStyle().Padding(1, 2)
	TitleStyle      = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF79C6")).Bold(true)
	PromptStyle     = lipgloss.NewStyle().Foreground(lipgloss.Color("#50FA7B"))
	UserMsgStyle    = lipgloss.NewStyle().Foreground(lipgloss.Color("#F1FA8C"))
	AISystemStyle   = lipgloss.NewStyle().Foreground(lipgloss.Color("#BD93F9"))
	EngineRespStyle = lipgloss.NewStyle().Foreground(lipgloss.Color("#8BE9FD")).PaddingLeft(2)
	ErrorStyle      = lipgloss.NewStyle().Foreground(lipgloss.Color("#FF5555"))
	StatusStyle     = lipgloss.NewStyle().Foreground(lipgloss.Color("#6272A4"))
	InputStyle      = lipgloss.NewStyle().Foreground(lipgloss.Color("#F8F8F2")).Padding(0, 1).Border(lipgloss.RoundedBorder()).BorderForeground(lipgloss.Color("#44475A"))
)
