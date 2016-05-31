using UnityEngine;
using UnityEditor;
using System.Collections;

[CustomEditor (typeof(BehaviourTree))]
public class BehaviourTreeEditor : Editor 
{
	BehaviourTree behaviourTree;


	public void OnEnable()
	{
		behaviourTree = (BehaviourTree)target;
	}

	// Override the default Editor
	public override void OnInspectorGUI()
	{
		string shortened = behaviourTree.GetCurrentFilePath();
		GUIStyle multilineStyle = new GUIStyle(); 
		multilineStyle.wordWrap = true; 
		multilineStyle.alignment = TextAnchor.UpperLeft;		
		Color c = Color.black; c.a = 1f; multilineStyle.normal.textColor = c;

		EditorGUILayout.LabelField("Current File Path", behaviourTree.GetCurrentFilePath(), multilineStyle);
		if (GUILayout.Button("Edit Behaviour Tree", GUILayout.Width(255)))
		{   
			BehaviourWindow window = (BehaviourWindow) EditorWindow.GetWindow(typeof(BehaviourWindow));
			window.Init();
		}
	}

}
