using Godot;
using System;
using System.Collections.Generic;

public partial class GUIController : CanvasLayer
{
    [Export] private SerialInterface _serialInterface;

    private Dictionary<string, Label> _rcAxisLabels = new();

    public override void _Ready()
    {
        _serialInterface = GetParent().GetNode<SerialInterface>("SerialConnection");
        
        var bottomBar = GetNode<Control>("BottomBar");

        foreach (var child in bottomBar.GetChildren())
        {
            if (child is Label label)
            {
                _rcAxisLabels[label.Name] = label;
            }
        }
    }
    
    public override void _Process(double delta)
    {
        _rcAxisLabels["Thrust"].Text = $"Thrust: {_serialInterface.RC_Input.W}";
        _rcAxisLabels["Roll"].Text = $"Roll: {_serialInterface.RC_Input.X}";
        _rcAxisLabels["Pitch"].Text = $"Pitch: {_serialInterface.RC_Input.Y}";
        _rcAxisLabels["Yaw"].Text = $"Yaw: {_serialInterface.RC_Input.Z}";
    }

}
