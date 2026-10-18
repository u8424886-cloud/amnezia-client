// Инструкция по внесению правок в форк amnezia-vpn/amnezia-client.
//
// Каждый пункт — конкретное место в репозитории и что именно поменять.
// Всё это нужно сделать в GitHub через веб-редактор (кнопка "Edit" у файла).

// =========================================================================
// 1. НОВЫЕ ФАЙЛЫ — положить как есть, содержимое готово
// =========================================================================
//
//   client/core/controllers/warpController.h
//   client/core/controllers/warpController.cpp
//   client/ui/qml/Pages2/PageWarpGenerator.qml
//
// CMake менять НЕ нужно: client/CMakeLists.txt использует
//   file(GLOB_RECURSE AMNEZIAVPN_QML_FILES .../ui/qml/*.qml)
// и подхватывает новые файлы сам.

// =========================================================================
// 2. client/ui/utils/pageEnum.h — добавить страницу в перечисление
// =========================================================================
//
// Найти строку
//         PageSetupWizardConfigSource,
// и добавить после неё
//         PageSetupWizardWarpGenerator,
// и рядом
//         PageSetupWizardWarpGenerator,
// в список (в конце блока перед "};" тоже годится).
//
// Без этого PageEnum.PageSetupWizardWarpGenerator в QML не найдётся.

// =========================================================================
// 3. client/amneziaApplication.cpp — создать контроллер
// =========================================================================
//
// Найти место, где создаются контроллеры (например, где new ServersUiController
// или make_unique<ServersUiController>), и рядом добавить:
//
//     auto warpController = new WarpController(this);
//     qmlRegisterSingletonInstance("WarpController", 1, 0,
//                                  "WarpController", warpController);
//
// Если контроллеры заводятся по-другому (через общий класс), сделать по его
// образцу: главное — чтобы объект жил дольше, чем QML, и был зарегистрирован
// под именем "WarpController".

// =========================================================================
// 4. client/ui/qml/Pages2/PageSetupWizardConfigSource.qml — меню
// =========================================================================
//
// 4а. Скрыть два ненужных пункта.
//
//      QtObject {
//          id: amneziaVpn
//          ...
//          property bool isVisible: true     ← заменить на false
//      }
//
//      QtObject {
//          id: selfHostVpn
//          ...
//          property bool isVisible: true     ← заменить на false
//      }
//
// 4б. Добавить свой пункт в список variants:
//
//      property list<QtObject> variants: [
//          warpGenerator,
//          amneziaVpn,
//          selfHostVpn,
//          backupRestore,
//          fileOpen,
//          qrScan,
//          restorePurchases,
//          siteLink
//      ]
//
// 4в. Описать сам пункт (рядом с другими QtObject):
//
//      QtObject {
//          id: warpGenerator
//
//          property bool featuredAmneziaConnection: false
//          property string title: qsTr("Сгенерировать WARP")
//          property string description: qsTr("Бесплатные конфигурации Cloudflare WARP")
//          property string imageSource: "qrc:/images/controls/plus.svg"
//          property bool isVisible: true
//          property var handler: function() {
//              PageController.goToPage(PageEnum.PageSetupWizardWarpGenerator)
//          }
//      }
//
// Иконку можно заменить на любую существующую в qrc:/images/controls/,
// например "server.svg" или "globe.svg".

// =========================================================================
// 5. client/ui/qml/Components/ConnectionTypeSelectionDrawer.qml — меню "Add"
// =========================================================================
//
// Это меню по кнопке "+" внизу. Сейчас там два пункта:
//
//      ListItemType { text: qsTr("Configure your server")      ← скрыть
//                     ... onClicked: goToPage(PageSetupWizardCredentials) }
//      ListItemType { text: qsTr("Open config file...")         ← оставить
//
// Первому блоку добавить  visible: false
// Второму добавить       visible: true
//
// Можно вместо этого дописать свой блок с переходом на генератор.

// =========================================================================
// 6. Сборка
// =========================================================================
//
// После всех правок — любой коммит в репозиторий, и deploy.yml соберёт
// Windows и Linux автоматически. Скачать: вкладка Actions → нужный запуск
// → Artifacts внизу.

// =========================================================================
// ИТОГОВЫЙ ИНТЕРФЕЙС
// =========================================================================
//
//   Кнопка "+" внизу  ->  "Сгенерировать WARP"
//                          ↓
//                     [Сгенерировать 5 конфигов]
//                          ↓  (5 запросов в Cloudflare, ~20-40 сек)
//                     WARP 1   188.114.97.11:1014      ->
//                     WARP 2   162.159.195.80:955      ->
//                     WARP 3   8.34.146.15:8886        ->
//                     WARP 4   8.34.146.45:3581        ->
//                     WARP 5   8.39.214.87:908         ->
//                          ↓  тап по строке
//                     экран просмотра конфига -> Подключиться
//
//   Плюс кнопка "Открыть папку с конфигами" — файлы лежат в
//   %APPDATA%\AmneziaVPN\WarpConfigs\
