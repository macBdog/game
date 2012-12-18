#include "DebugMenu.h"
#include "FontManager.h"
#include "RenderManager.h"

#include "GameObject.h"

using namespace std;	//< For fstream operations

bool GameObject::Update(float a_dt)
{
	// Tick the object's life
	if (m_state == eGameObjectState_Active)
	{
		m_lifeTime += a_dt;
	}

	return true;
}

bool GameObject::Draw()
{
	if (m_state == eGameObjectState_Active)
	{
		// Normal mesh rendering
		RenderManager & rMan = RenderManager::Get();

		if (m_model != NULL && m_model->IsLoaded())
		{
			rMan.AddModel(RenderManager::eBatchWorld, m_model, &m_worldMat);
		}
		
		// Draw the object's name, position, orientation and clip volume over the top
		if (DebugMenu::Get().IsDebugMenuEnabled())
		{
			rMan.AddDebugMatrix(m_worldMat);

			switch (m_clipType)
			{
				case eClipTypeSphere:
				{
					rMan.AddDebugSphere(m_worldMat.GetPos(), m_clipVolumeSize.GetX(), sc_colourPurple); 
					break;
				}
				case eClipTypeCube:
				{
					rMan.AddDebugCube(m_worldMat.GetPos(), m_clipVolumeSize.GetX(), sc_colourPurple); 
				}
				default: break;
			}

			FontManager::Get().DrawDebugString3D(m_name, 1.0f, m_worldMat.GetPos());
		}

		return true;
	}

	return false;
}


void GameObject::Serialise(ofstream * a_outputStream, unsigned int a_indentCount)
{
	char outBuf[StringUtils::s_maxCharsPerName];
	const char * lineEnd = "\n";
	const char * tab = "\t";

	// If this is a recursive call the output stream will be already set up
	if (a_outputStream != NULL)
	{
		// Generate the correct tab amount
		char tabs[StringUtils::s_maxCharsPerName];
		memset(&tabs[0], '\t', a_indentCount);
		tabs[a_indentCount] = '\0';

		ofstream & menuStream = *a_outputStream;
		menuStream << tabs << "gameObject" << lineEnd;
		menuStream << tabs << "{" << lineEnd;
		menuStream << tabs << tab << "name: "	<< m_name				<< lineEnd;
		
		m_worldMat.GetPos().GetString(outBuf);
		menuStream << tabs << tab << "pos: "	<< outBuf	<< lineEnd;

		menuStream << tabs << "}" << lineEnd;

		// Serialise any siblings of this element at this indentation level
		GameObject * next = m_next;
		while (next != NULL)
		{
			next->Serialise(a_outputStream, a_indentCount);
			next = next->GetNext();
		}

		// Serialise any children of this child
		GameObject * child = m_child;
		while (child != NULL)
		{
			child->Serialise(a_outputStream, ++a_indentCount);
			child = child->GetChild();
		}
	}
}
