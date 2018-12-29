"resource/ui/TASPanel.res"
{
	"TASPanel"
	{
		"ControlName"		"TASPanel"
		"fieldName"		"TASPanel"
		"xpos"		"400"
		"ypos"		"100"
		"wide"		"190"
		"tall"		"210"
		"autoResize"		"0"
		"pinCorner"		"0"
		"tabPosition"		"0"
	}

	"EnableTASMode"
	{
		"ControlName"		"ToggleButton"
		"fieldName"		"EnableTASMode"
		"xpos"		"10"
		"ypos"		"40"
		"wide"		"170"
		"tall"		"16"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#MOM_DisabledTasMode"
		"textAlignment"		"center"
		"dulltext"		"0"
        "font" "DefaultSmall"
		"brighttext"		"0"
		"wrap"		"0"
		"Command"		"enabletasmode"
		"Default"		"0"
        "mouseinputenabled" "1"
	}

    "VisPredMove"
    {
        "ControlName" "ToggleButton"
        "fieldName" "VisPredMove"
        "labelText"		"#MOM_VisPredMove"
		"xpos"		"10"
		"ypos"		"80"
		"wide"		"170"
		"tall"		"16"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"center"
		"dulltext"		"0"
        "font" "DefaultSmall"
		"brighttext"		"0"
		"wrap"		"0"
		"Command"		"vispredmove"
		"Default"		"0"
        "mouseinputenabled" "1"
    }

    "Autostrafe"
    {
        "ControlName" "ToggleButton"
        "fieldName" "Autostrafe"
        "labelText"		"#MOM_Autostrafe"
		"xpos"		"10"
		"ypos"		"120"
		"wide"		"170"
		"tall"		"16"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"textAlignment"		"center"
		"dulltext"		"0"
        "font" "DefaultSmall"
		"brighttext"		"0"
		"wrap"		"0"
		"Command"		"autostrafe"
		"Default"		"0"
        "mouseinputenabled" "1"
    }

	"ToggleReplayUI"
	{
		"ControlName"		"ToggleButton"
		"fieldName"		"ToggleReplayUI"
		"xpos"		"10"
		"ypos"		"160"
		"wide"		"170"
		"tall"		"16"
		"autoResize"		"0"
		"pinCorner"		"0"
		"visible"		"1"
		"enabled"		"1"
		"tabPosition"		"0"
		"labelText"		"#MOM_TASReplayUI"
		"textAlignment"		"center"
		"dulltext"		"0"
        "font" "DefaultSmall"
		"brighttext"		"0"
		"wrap"		"0"
		"Default"		"0"
        "mouseinputenabled" "1"
	}
}