module.exports = [
  {
    "type": "heading",
    "defaultValue": "Big Date Settings"
  },
  {
    "type": "text",
    "defaultValue": "Configure how the app behaves while it is open."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Auto-Quit (Quick Launch)"
      },
      {
        "type": "select",
        "messageKey": "QuitTimeout",
        "defaultValue": "0",
        "label": "Quit after (quick launch only)",
        "description": "When the app is opened via quick launch, automatically close it and return to the watchface after this amount of time. Has no effect when opened from the launcher menu.",
        "options": [
          { "label": "Never", "value": "0" },
          { "label": "5 seconds", "value": "5" },
          { "label": "10 seconds", "value": "10" },
          { "label": "15 seconds", "value": "15" },
          { "label": "30 seconds", "value": "30" },
          { "label": "60 seconds", "value": "60" }
        ]
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
