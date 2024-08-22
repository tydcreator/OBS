#include "window-basic-main.hpp"
#include <qt-wrappers.hpp>
#include <sstream>
#include <QColorDialog>

QMenu *OBSBasic::CreateDeinterlacingMenu(obs_source_t *source)
{
	QMenu *menu = new QMenu(QTStr("Deinterlacing"));

	obs_deinterlace_mode deinterlaceMode =
		obs_source_get_deinterlace_mode(source);
	obs_deinterlace_field_order deinterlaceOrder =
		obs_source_get_deinterlace_field_order(source);
	QAction *action;

	auto addMode = [&](const QString &name, obs_deinterlace_mode mode) {
		action = menu->addAction(name, this, [mode, source]() {
			obs_source_set_deinterlace_mode(source, mode);
		});
		action->setCheckable(true);
		action->setChecked(deinterlaceMode == mode);
	};

	addMode(QTStr("Disable"), OBS_DEINTERLACE_MODE_DISABLE);
	addMode(QTStr("Deinterlacing.Discard"), OBS_DEINTERLACE_MODE_DISCARD);
	addMode(QTStr("Deinterlacing.Retro"), OBS_DEINTERLACE_MODE_RETRO);
	addMode(QTStr("Deinterlacing.Blend"), OBS_DEINTERLACE_MODE_BLEND);
	addMode(QTStr("Deinterlacing.Blend2x"), OBS_DEINTERLACE_MODE_BLEND_2X);
	addMode(QTStr("Deinterlacing.Linear"), OBS_DEINTERLACE_MODE_LINEAR);
	addMode(QTStr("Deinterlacing.Linear2x"),
		OBS_DEINTERLACE_MODE_LINEAR_2X);
	addMode(QTStr("Deinterlacing.Yadif"), OBS_DEINTERLACE_MODE_YADIF);
	addMode(QTStr("Deinterlacing.Yadif2x"), OBS_DEINTERLACE_MODE_YADIF_2X);

	menu->addSeparator();

	auto addOrder = [&](const QString &name,
			    obs_deinterlace_field_order order) {
		action = menu->addAction(name, this, [order, source]() {
			obs_source_set_deinterlace_field_order(source, order);
		});
		action->setCheckable(true);
		action->setChecked(deinterlaceOrder == order);
	};

	addOrder(QTStr("TopFieldFirst"), OBS_DEINTERLACE_FIELD_ORDER_TOP);
	addOrder(QTStr("BottomFieldFirst"), OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);

	return menu;
}

QMenu *OBSBasic::CreateScaleFilteringMenu(obs_sceneitem_t *item)
{
	QMenu *menu = new QMenu(QTStr("ScaleFiltering"));

	obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);
	QAction *action;

	auto addMode = [&](const QString &name, obs_scale_type mode) {
		action = menu->addAction(name, this, [mode, item]() {
			obs_sceneitem_set_scale_filter(item, mode);
		});
		action->setCheckable(true);
		action->setChecked(scaleFilter == mode);
	};

	addMode(QTStr("Disable"), OBS_SCALE_DISABLE);
	addMode(QTStr("ScaleFiltering.Point"), OBS_SCALE_POINT);
	addMode(QTStr("ScaleFiltering.Bilinear"), OBS_SCALE_BILINEAR);
	addMode(QTStr("ScaleFiltering.Bicubic"), OBS_SCALE_BICUBIC);
	addMode(QTStr("ScaleFiltering.Lanczos"), OBS_SCALE_LANCZOS);
	addMode(QTStr("ScaleFiltering.Area"), OBS_SCALE_AREA);

	return menu;
}

QMenu *OBSBasic::CreateBlendingMethodMenu(obs_sceneitem_t *item)
{
	QMenu *menu = new QMenu(QTStr("BlendingMethod"));

	obs_blending_method blendingMethod =
		obs_sceneitem_get_blending_method(item);
	QAction *action;

	auto addMode = [&](const QString &name, obs_blending_method method) {
		action = menu->addAction(name, this, [method, item]() {
			obs_sceneitem_set_blending_method(item, method);
		});
		action->setCheckable(true);
		action->setChecked(blendingMethod == method);
	};

	addMode(QTStr("BlendingMethod.Default"), OBS_BLEND_METHOD_DEFAULT);
	addMode(QTStr("BlendingMethod.SrgbOff"), OBS_BLEND_METHOD_SRGB_OFF);

	return menu;
}

QMenu *OBSBasic::CreateBlendingModeMenu(obs_sceneitem_t *item)
{
	QMenu *menu = new QMenu(QTStr("BlendingMode"));

	obs_blending_type blendingMode = obs_sceneitem_get_blending_mode(item);
	QAction *action;

	auto addMode = [&](const QString &name, obs_blending_type mode) {
		action = menu->addAction(name, this, [mode, item]() {
			obs_sceneitem_set_blending_mode(item, mode);
		});
		action->setCheckable(true);
		action->setChecked(blendingMode == mode);
	};

	addMode(QTStr("BlendingMode.Normal"), OBS_BLEND_NORMAL);
	addMode(QTStr("BlendingMode.Additive"), OBS_BLEND_ADDITIVE);
	addMode(QTStr("BlendingMode.Subtract"), OBS_BLEND_SUBTRACT);
	addMode(QTStr("BlendingMode.Screen"), OBS_BLEND_SCREEN);
	addMode(QTStr("BlendingMode.Multiply"), OBS_BLEND_MULTIPLY);
	addMode(QTStr("BlendingMode.Lighten"), OBS_BLEND_LIGHTEN);
	addMode(QTStr("BlendingMode.Darken"), OBS_BLEND_DARKEN);

	return menu;
}

static void ConfirmColor(SourceTree *sources, const QColor &color,
			 QModelIndexList selectedItems)
{
	for (int x = 0; x < selectedItems.count(); x++) {
		SourceTreeItem *treeItem =
			sources->GetItemWidget(selectedItems[x].row());
		treeItem->setStyleSheet("background: " +
					color.name(QColor::HexArgb));
		treeItem->style()->unpolish(treeItem);
		treeItem->style()->polish(treeItem);

		OBSSceneItem sceneItem = sources->Get(selectedItems[x].row());
		OBSDataAutoRelease privData =
			obs_sceneitem_get_private_settings(sceneItem);
		obs_data_set_int(privData, "color-preset", 1);
		obs_data_set_string(privData, "color",
				    QT_TO_UTF8(color.name(QColor::HexArgb)));
	}
}

void OBSBasic::ColorButtonClicked(QPushButton *button, int preset)
{
	QModelIndexList selectedItems =
		ui->sources->selectionModel()->selectedIndexes();

	if (selectedItems.count() == 0)
		return;

	for (int x = 0; x < selectedItems.count(); x++) {
		SourceTreeItem *treeItem =
			ui->sources->GetItemWidget(selectedItems[x].row());
		treeItem->setStyleSheet("");
		treeItem->setProperty("bgColor", preset);
		treeItem->style()->unpolish(treeItem);
		treeItem->style()->polish(treeItem);

		OBSSceneItem sceneItem =
			ui->sources->Get(selectedItems[x].row());
		OBSDataAutoRelease privData =
			obs_sceneitem_get_private_settings(sceneItem);
		obs_data_set_int(privData, "color-preset", preset + 1);
		obs_data_set_string(privData, "color", "");
	}

	for (int i = 1; i < 9; i++) {
		std::stringstream preset;
		preset << "preset" << i;
		QPushButton *cButton =
			button->parentWidget()->findChild<QPushButton *>(
				preset.str().c_str());
		cButton->setStyleSheet("border: 1px solid black");
	}

	button->setStyleSheet("border: 2px solid black");
}

void OBSBasic::CustomColorSelected()
{
	QModelIndexList selectedItems =
		ui->sources->selectionModel()->selectedIndexes();

	if (selectedItems.count() == 0)
		return;

	OBSSceneItem curSceneItem = GetCurrentSceneItem();
	SourceTreeItem *curTreeItem = GetItemWidgetFromSceneItem(curSceneItem);
	OBSDataAutoRelease curPrivData =
		obs_sceneitem_get_private_settings(curSceneItem);

	int oldPreset = obs_data_get_int(curPrivData, "color-preset");
	const QString oldSheet = curTreeItem->styleSheet();

	auto liveChangeColor = [=](const QColor &color) {
		if (color.isValid()) {
			curTreeItem->setStyleSheet("background: " +
						   color.name(QColor::HexArgb));
		}
	};

	auto changedColor = [=](const QColor &color) {
		if (color.isValid()) {
			ConfirmColor(ui->sources, color, selectedItems);
		}
	};

	auto rejected = [=]() {
		if (oldPreset == 1) {
			curTreeItem->setStyleSheet(oldSheet);
			curTreeItem->setProperty("bgColor", 0);
		} else if (oldPreset == 0) {
			curTreeItem->setStyleSheet("background: none");
			curTreeItem->setProperty("bgColor", 0);
		} else {
			curTreeItem->setStyleSheet("");
			curTreeItem->setProperty("bgColor", oldPreset - 1);
		}

		curTreeItem->style()->unpolish(curTreeItem);
		curTreeItem->style()->polish(curTreeItem);
	};

	QColorDialog::ColorDialogOptions options =
		QColorDialog::ShowAlphaChannel;

	const char *oldColor = obs_data_get_string(curPrivData, "color");
	const char *customColor = *oldColor != 0 ? oldColor : "#55FF0000";
#ifdef __linux__
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	QColorDialog colorDialog(this);
	colorDialog.setOptions(options);
	colorDialog.setCurrentColor(QColor(customColor));
	connect(&colorDialog, &QColorDialog::currentColorChanged,
		liveChangeColor);
	connect(&colorDialog, &QColorDialog::colorSelected, changedColor);
	connect(&colorDialog, &QColorDialog::rejected, rejected);
	colorDialog.exec();
}

void OBSBasic::ClearColors()
{
	QModelIndexList selectedItems =
		ui->sources->selectionModel()->selectedIndexes();

	if (selectedItems.count() == 0)
		return;

	for (int x = 0; x < selectedItems.count(); x++) {
		SourceTreeItem *treeItem =
			ui->sources->GetItemWidget(selectedItems[x].row());
		treeItem->setStyleSheet("background: none");
		treeItem->setProperty("bgColor", 0);
		treeItem->style()->unpolish(treeItem);
		treeItem->style()->polish(treeItem);

		OBSSceneItem sceneItem =
			ui->sources->Get(selectedItems[x].row());
		OBSDataAutoRelease privData =
			obs_sceneitem_get_private_settings(sceneItem);
		obs_data_set_int(privData, "color-preset", 0);
		obs_data_set_string(privData, "color", "");
	}
}

QMenu *OBSBasic::CreateBackgroundColorMenu(obs_sceneitem_t *item)
{
	QMenu *menu = new QMenu(QTStr("ChangeBG"));
	QWidgetAction *widgetAction = new QWidgetAction(menu);
	ColorSelect *select = new ColorSelect(menu);

	QAction *action;

	menu->setStyleSheet(QString(
		"*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
		"*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
		"*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
		"*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
		"*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
		"*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
		"*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
		"*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

	OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(item);

	obs_data_set_default_int(privData, "color-preset", 0);
	int preset = obs_data_get_int(privData, "color-preset");

	action = menu->addAction(QTStr("Clear"), this, &OBSBasic::ClearColors);
	action->setCheckable(true);
	action->setChecked(preset == 0);

	action = menu->addAction(QTStr("CustomColor"), this,
				 &OBSBasic::CustomColorSelected);
	action->setCheckable(true);
	action->setChecked(preset == 1);

	menu->addSeparator();

	widgetAction->setDefaultWidget(select);

	for (int i = 1; i < 9; i++) {
		std::stringstream button;
		button << "preset" << i;
		QPushButton *colorButton =
			select->findChild<QPushButton *>(button.str().c_str());
		colorButton->setProperty("bgColor", i);
		if (preset == i + 1)
			colorButton->setStyleSheet("border: 2px solid black");

		select->connect(colorButton, &QPushButton::released, this,
				[this, colorButton, i]() {
					ColorButtonClicked(colorButton, i);
				});
	}

	menu->addAction(widgetAction);

	return menu;
}

ColorSelect::ColorSelect(QWidget *parent)
	: QWidget(parent),
	  ui(new Ui::ColorSelect)
{
	ui->setupUi(this);
}
