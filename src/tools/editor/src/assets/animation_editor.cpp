#include "animation_editor.h"
#include "halley/tools/project/project.h"
#include "halley/ui/widgets/ui_animation.h"
#include "halley/ui/widgets/ui_dropdown.h"
#include "src/ui/scroll_background.h"

using namespace Halley;

AnimationEditor::AnimationEditor(UIFactory& factory, Resources& resources, Project& project, const String& assetId, AssetType type)
	: UIWidget("animationEditor", {}, UISizer())
	, factory(factory)
	, project(project)
{
	if (type == AssetType::Animation) {
		animation = resources.get<Animation>(assetId);
	} else if (type == AssetType::Sprite) {
		sprite = resources.get<SpriteResource>(assetId);
	}
	setupWindow();
}

void AnimationEditor::setupWindow()
{
	add(factory.makeUI("ui/halley/animation_editor"), 1);

	auto animationDisplay = getWidgetAs<AnimationEditorDisplay>("display");
	if (animation) {
		animationDisplay->setAnimation(animation);
	} else {
		animationDisplay->setSprite(sprite);
	}
	getWidgetAs<ScrollBackground>("scrollBackground")->setZoomListener([=] (float zoom)
	{
		animationDisplay->setZoom(zoom);
	});

	if (animation) {
		auto sequenceList = getWidgetAs<UIDropdown>("sequence");
		sequenceList->setOptions(animation->getSequenceNames());

		auto directionList = getWidgetAs<UIDropdown>("direction");
		directionList->setOptions(animation->getDirectionNames());
	} else {
		getWidget("animControls")->setActive(false);
	}

	setHandle(UIEventType::DropboxSelectionChanged, "sequence", [=] (const UIEvent& event)
	{
		animationDisplay->setSequence(event.getData());
	});

	setHandle(UIEventType::DropboxSelectionChanged, "direction", [=] (const UIEvent& event)
	{
		animationDisplay->setDirection(event.getData());
	});
}

AnimationEditorDisplay::AnimationEditorDisplay(String id, Resources& resources)
	: UIWidget(std::move(id))
	, resources(resources)
{
	boundsSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceVSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	nineSliceHSprite.setImage(resources, "whitebox_outline.png").setColour(Colour4f(0, 1, 0));
	pivotSprite.setImage(resources, "ui/pivot.png").setColour(Colour4f(1, 0, 1));
}

void AnimationEditorDisplay::setZoom(float z)
{
	zoom = z;
	updateAnimation();
}

void AnimationEditorDisplay::setAnimation(std::shared_ptr<const Animation> a)
{
	animation = std::move(a);
	animationPlayer.setAnimation(animation);
	updateAnimation();
}

void AnimationEditorDisplay::setSprite(std::shared_ptr<const SpriteResource> s)
{
	sprite = std::move(s);
}

void AnimationEditorDisplay::setSequence(const String& sequence)
{
	animationPlayer.setSequence(sequence);
}

void AnimationEditorDisplay::setDirection(const String& direction)
{
	animationPlayer.setDirection(direction);
}

void AnimationEditorDisplay::update(Time t, bool moved)
{
	updateAnimation();

	if (animation) {
		animationPlayer.update(t);
		animationPlayer.updateSprite(origSprite);
	}

	const Vector2f pivotPos = getPosition() - bounds.getTopLeft() * zoom;

	drawSprite = origSprite.clone().setPos(pivotPos).setScale(zoom).setNotSliced();
	pivotSprite.setPos(pivotPos);
	boundsSprite.setPos(getPosition()).scaleTo(bounds.getSize() * zoom);

	if (origSprite.isSliced()) {
		auto slices = Vector4f(origSprite.getSlices());
		nineSliceVSprite.setVisible(true).setPos(getPosition() + Vector2f(0, slices.y) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(0, slices.w + slices.y)) * zoom));
		nineSliceHSprite.setVisible(true).setPos(getPosition() + Vector2f(slices.x, 0) * zoom).scaleTo(Vector2f::max(Vector2f(1, 1), (bounds.getSize() - Vector2f(slices.x + slices.z, 0)) * zoom));
	} else {
		nineSliceVSprite.setVisible(false);
		nineSliceHSprite.setVisible(false);
	}
}

void AnimationEditorDisplay::draw(UIPainter& painter) const
{
	painter.draw(drawSprite);
	painter.draw(boundsSprite);
	painter.draw(pivotSprite);
	if (nineSliceHSprite.isVisible()) {
		painter.draw(nineSliceHSprite);
	}
	if (nineSliceVSprite.isVisible()) {
		painter.draw(nineSliceVSprite);
	}
}

void AnimationEditorDisplay::updateAnimation()
{
	if (animation) {
		bounds = Rect4f(animation->getBounds());
	} else {
		origSprite.setImage(*sprite, resources.get<MaterialDefinition>("Halley/Sprite"));
		const auto origin = -origSprite.getAbsolutePivot() - Vector2f(origSprite.getOuterBorder().xy());
		const auto sz = origSprite.getOriginalSize();
		bounds = Rect4f(origin, origin + sz);
	}

	setMinSize(bounds.getSize() * zoom);
}