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
	
	public Quaternion Attitude = Quaternion.Identity;
	public Quaternion TargetAttitude = Quaternion.Identity;
	public Vector4 RC_Input = Vector4.Zero;
	public Vector4 Thrust = Vector4.Zero;
	
	// Called when the node enters the scene tree for the first time.
	public override void _Ready()
	{
		_serialPort = new SerialPort(_portName, _baudRate); // Port name and baud rate
		_serialPort.Open();
	}

	// Called every frame. 'delta' is the elapsed time since the previous frame.
	public override void _Process(double delta)
	{
		if (_serialPort == null || !_serialPort.IsOpen || _serialPort.BytesToRead <= 0) return;
		var data = _serialPort.ReadExisting();

		var args = data.Split("\n\r", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
		foreach (var arg in args)
		{
			var lineArgs = arg.Split(':', StringSplitOptions.TrimEntries);
			float temp;
			switch (lineArgs[0])
			{
				case "FC_Attitude":
				{
					var q = lineArgs[1].Split(",", StringSplitOptions.TrimEntries);

					if (float.TryParse(q[0], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Attitude.W = temp;
					}

					if (float.TryParse(q[1], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Attitude.X = -temp;
					}

					if (float.TryParse(q[2], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Attitude.Z = -temp;
					}

					if (float.TryParse(q[3], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Attitude.Y = temp;
					}

					break;
				}
				case "FC_TAttitude":
				{
					var q = lineArgs[1].Split(",", StringSplitOptions.TrimEntries);

					if (float.TryParse(q[0], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						TargetAttitude.W = temp;
					}

					if (float.TryParse(q[1], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						TargetAttitude.X = -temp;
					}

					if (float.TryParse(q[2], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						TargetAttitude.Z = -temp;
					}

					if (float.TryParse(q[3], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						TargetAttitude.Y = temp;
					}
					
					break;
				}
				case "FC_RC_AXES":
				{
					var input = lineArgs[1].Split(",", StringSplitOptions.TrimEntries);

					if (float.TryParse(input[0], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						RC_Input.W = temp;
					}

					if (float.TryParse(input[1], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						RC_Input.X = temp;
					}

					if (float.TryParse(input[2], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						RC_Input.Y = temp;
					}

					if (float.TryParse(input[3], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						RC_Input.Z = temp;
					}

					break;
				}
				case "FC_Thrust":
				{
					var input = lineArgs[1].Split(",", StringSplitOptions.TrimEntries);

					if (float.TryParse(input[0], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Thrust.W = temp;
					}

					if (float.TryParse(input[1], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Thrust.X = temp;
					}

					if (float.TryParse(input[2], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Thrust.Y = temp;
					}

					if (float.TryParse(input[3], NumberStyles.Float, CultureInfo.InvariantCulture, out temp))
					{
						Thrust.Z = temp;
					}

					break;
				}
				case "FC_TargetV":
				{
					GD.Print(arg);
					break;
				}
				case "Here":
				{
					GD.Print(arg);
					break;
				}
				default:
				{
					if(!Data.ContainsKey(lineArgs[0]))
					{
						GD.Print("New: " + lineArgs[0]);
					}
					
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
		if (_serialPort is { IsOpen: true })
		{
			_serialPort.Close();
		}
	}
}
