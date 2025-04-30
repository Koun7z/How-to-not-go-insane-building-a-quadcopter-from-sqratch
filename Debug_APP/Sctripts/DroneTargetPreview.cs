using Godot;
using System;

public partial class DroneTargetPreview : StaticBody3D
{
    [Export] private SerialInterface _serialConnection;
    
    public override void _Ready()
    {
        
    }

    public override void _Process(double delta)
    {
        Quaternion q = _serialConnection.TargetAttitude.Normalized();

        
        Basis basis = new(q.Normalized());
        
        //GD.Print(basis.GetEuler(EulerOrder.Yzx));
        
        this.Basis = basis;
    }
}
