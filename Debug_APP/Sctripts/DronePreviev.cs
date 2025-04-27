using Godot;
using System;

public partial class DronePreviev : StaticBody3D
{
    [Export] private SerialInterface _serialConnection;
    
    public override void _Ready()
    {
        
    }

    public override void _Process(double delta)
    {
        Quaternion q = _serialConnection.Attitude.Normalized().Inverse();
        
        Vector3 up = new Vector3(0, 1, 0);

        up = q * up;

        Quaternion q_out;
        
        q_out.W = Mathf.Sqrt(0.5f * (1.0f + up.Y));  // careful: up_z could be -1 if upside down
        if(q_out.W > 1e-6f) // Protect against divide by zero
        {
            q_out.X = up.Z / (2.0f * q_out.W);
            q_out.Z = -up.X / (2.0f * q_out.W);
        }
        else
        {
            // When facing straight down (up_z = -1), roll/pitch are undefined
            q_out.X = -1.0f;
            q_out.Z = 0.0f;
        }
        q_out.Y = 0.0f;

        
        Basis basis = new(q_out.Normalized());
        
        //GD.Print(basis.GetEuler(EulerOrder.Yzx));
        
        this.Basis = basis;
    }
}
