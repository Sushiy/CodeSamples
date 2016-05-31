using UnityEngine;
using System.Collections;

public interface Node
{
	void Create(int nodeID, Vector2 guiPosition, ParentNode parent, BehaviourTree tree);
	GUINode GetView();
	int GetTypeID();
	void Delete();
}

public interface ChildNode : Node
{
	void Activate();	
	void Deactivate();

	ParentNode GetParent();
}

public interface ParentNode : Node
{
	void AddChild(ChildNode child);
	void RemoveChild(ChildNode child);
	void ChildDone(ChildNode child, bool childResult);
	void ChildEvent(ChildNode child);
}

public interface TaskNode : ChildNode
{
	void PerformTask();
}

public interface ConditionNode: ChildNode
{
	void Check();
}

public interface DecoratorNode: ParentNode, ChildNode
{

}


