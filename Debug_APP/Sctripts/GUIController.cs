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
        
        var drone = GetNode("BottomBar/DroneSprite");

        foreach (var child in drone.GetChildren())
        {
            if (child is Label label)
            {
                GD.Print(label.Name);
                _rcAxisLabels[label.Name] = label;
            }
        }
        
    }
    
    public override void _Process(double delta)
    {
        _rcAxisLabels["Thrust"].Text = $"Thrust: {_serialInterface.RC_Input.W:F3}";
        _rcAxisLabels["Roll"].Text = $"Roll: {_serialInterface.RC_Input.X:F3}";
        _rcAxisLabels["Pitch"].Text = $"Pitch: {_serialInterface.RC_Input.Y:F3}";
        _rcAxisLabels["Yaw"].Text = $"Yaw: {_serialInterface.RC_Input.Z}";
        
        _rcAxisLabels["Motor1"].Text = $"{_serialInterface.Thrust.W:F3}%";
        _rcAxisLabels["Motor2"].Text = $"{_serialInterface.Thrust.X:F3}%";
        _rcAxisLabels["Motor3"].Text = $"{_serialInterface.Thrust.Y:F3}%";
        _rcAxisLabels["Motor4"].Text = $"{_serialInterface.Thrust.Z:F3}%";
    }

}
