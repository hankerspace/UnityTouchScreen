using UnityEngine;
using System.Collections;
using System;
using System.Runtime.InteropServices;

using System.Diagnostics;
using Debug = UnityEngine.Debug;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
public class tTouchData
{
    public int m_x;
    public int m_y;
    public int m_ID;
    public int m_Time;
};

[StructLayout(LayoutKind.Sequential, Pack = 1)]

public class Check1 : MonoBehaviour {
	private bool m_Initialised;
    [DllImport("TouchDLL", CharSet = CharSet.Auto, CallingConvention = CallingConvention.StdCall)]
    public static extern int Initialise(string Str);

    [DllImport("TouchDLL")]
    public static extern void SetDebugFunction(IntPtr fp);

	[DllImport("TouchDLL")]
	public static extern int GetTouchPointCount();
	[DllImport ("TouchDLL")]
	public static extern void GetTouchPoint(int i, tTouchData n);

    static void CallBackFunction(string str)
    {
        Debug.Log("::DLLCallback : " + str);
    }

	// Use this for initialization
	void Start () {
		m_Initialised = false;
	}
	
	// Update is called once per frame
	void Update () {
	
	}
	
	void OnGUI () {

		string Str;
		int NumTouch = 0;
		if (!m_Initialised)
		{
			Str = "Touch";
			if (Initialise(Str) < 0)
			{
				// ERROR STATE
				Debug.LogError("Cant find unity process");
			}
			m_Initialised = true;
		}

		NumTouch = GetTouchPointCount();
		Str = "Number of Touch Points: " + NumTouch.ToString();
		GUI.Label (new Rect (10,10,150,40), Str);
		for (int p=0; p<NumTouch; p++)
		{
			tTouchData TouchData = new tTouchData();
			GetTouchPoint (p, TouchData);
			GUI.Label (new Rect (10,10 + (p+1) * 40, 200, 40), 
				"ID:" + TouchData.m_ID + 
				"Time:" + TouchData.m_Time.ToString() + 
				"(" + TouchData.m_x.ToString() + "," + TouchData.m_y.ToString() + ")");
			
		}
	}	
}
