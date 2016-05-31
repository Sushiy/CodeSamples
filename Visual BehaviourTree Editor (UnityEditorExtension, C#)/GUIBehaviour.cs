using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GUIBehaviour : GUINode 
{
	Behaviour model;
	
	public GUIBehaviour(int value, Vector2 position) : base("Behaviour", value, position)
	{
		baseColor = Color.cyan;
		TypeID = 0;
	}

	public override void DrawParentConnector()
	{
		//no parent
	}
	
	public override void DrawParentLine()
	{
		//no parent
	}

	public void SetModel(Behaviour model)
	{
		this.model = model;
	}

	public override Node GetModel()
	{
		return model;
	}

	public override bool CanHaveMoreChildren ()
	{
		return !(model.hasRoot());
	}

	public override List<GUINode> GetAllChildren ()
	{
		List<GUINode> children = new List<GUINode>();

		if(model.hasRoot())
		{
			children.Add(model.GetRoot().GetView());
			foreach(GUINode n in model.GetRoot().GetView().GetAllChildren())
			{
				children.Add(n);
			}
		}
		return children;
	}
}