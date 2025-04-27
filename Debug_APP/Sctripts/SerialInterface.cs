using Godot;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO.Ports;
using System.Runtime.InteropServices;

public partial class SerialInterface : Node
{
	private SerialPort _serialPort;
	
	[Export] private String _portName = "COM7";
	[Export] private int _baudRate = 115200;
	
	public Dictionary<String, float> Data = new();
	
	[Export] public Quaternion Attitude = Quaternion.Identity;

	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		_serialPort = new SerialPort(_portName, _baudRate); // Port name and baud rate
		_serialPort.Open();
	}

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{
		if (_serialPort != null && _serialPort.IsOpen && _serialPort.BytesToRead > 0)
		{
			var data = _serialPort.ReadExisting();

			var args = data.Split("\n\r", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
			foreach (var arg in args)
			{
				var lineArgs = arg.Split(':', StringSplitOptions.TrimEntries);
				float temp;
				switch (lineArgs[0])
				{ 
					case "FC_Attitude":

						var q = lineArgs[1].Split(",", StringSplitOptions.TrimEntries);
						
						if (float.TryParse(q[0], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
						{
							Attitude.W = temp;
						}

						if (float.TryParse(q[1],NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
						{
							Attitude.X = temp;
						}

						if (float.TryParse(q[2],NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
						{
							Attitude.Z = temp;
						}

						if (float.TryParse(q[3],NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
						{
							Attitude.Y = temp;
						}
						break;
					
					case "Here":
						
						GD.Print(arg);
						break;
					
					default:
						if (float.TryParse(lineArgs[1], out temp))
						{
							Data[lineArgs[0]] = temp;
						}
						break;
				}
			}
		}
	}
	
	public override void _ExitTree()
	{
		if (_serialPort != null && _serialPort.IsOpen)
		{
			_serialPort.Close();
		}
	}
}
