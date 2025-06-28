using Godot;
using System;

public partial class DronePreview : StaticBody3D
{
    [Export] private SerialInterface _serialConnection;
    
    public override void _Ready()
    {
        
    }

    public override void _Process(double delta)
    {
        Quaternion q = _serialConnection.Attitude.Normalized();
        
        Vector3 l = new Vector3(1, 0, 0);

        l = q * l;

        Quaternion q_out = new();
        
        /*q_out.W = Mathf.Sqrt(0.5f * (1.0f + up.Y));  // careful: up_z could be -1 if upside down
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
        q_out.Y = 0.0f;*/
        
        /*float temp = Mathf.Sqrt(l.X * l.X + l.Z * l.Z);
        
        q_out.W = Mathf.Sqrt(temp + l.X * Mathf.Sqrt(temp)) / Mathf.Sqrt(2 * temp);
        q_out.X = 0;
        q_out.Z = 0;
        q_out.Y = l.Z / Mathf.Sqrt(2 * (temp + l.X * Mathf.Sqrt(temp)));
        
        Basis basis = new(q * q_out);*/
        
        //GD.Print(basis.GetEuler(EulerOrder.Yzx));
        
        this.Basis = new Basis(q);
    }
}  
