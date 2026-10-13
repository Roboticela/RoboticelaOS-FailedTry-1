The filenames must be PascalCase. The functions, classes, and structs must be PascalCase. Variable names must be camelCase. Constants must be CAPITAL_CASE.

# Components:
Screen -> Screen will be the page in browser which will be identified uniquely for example /login and in native it will be also one screen which show in the window like login screen. The user can create multiple screens and even at once they can open multiple screens like how the paint tool has tools screen, main screen etc at the same time. The screen will have also the same layout options, we can grow it, we can align it and so on.
Board -> The board will not be containing anything by default it will be just a plain board. user can put anything inside it. It will be like a canvas. It will be having all other components and then user can connect any board with any screen to display things. It will be the main component for layout.
✅ - Container (clipped scroll region; layout stack, flex, grid, or position). `when="id"` shows that container only while the PaneItem with the same `id` is selected.
✅ - Text (This component is going to support the rich text with multiple font sizes at same text and multiple colors. Options like wrap, left, center, right etc. Basically, all the features which text supports must come in this component and user must be able to tweak and play with the component using XML code.)
✅ - Heading (1 to 6; derives from Text with semantic size/weight defaults)
✅ - Paragraph (derives from Text with body copy defaults)
✅ - Button (foundation variants filled/primary, tinted, gray/secondary, plain/tertiary, destructive; hover, press, focus, disabled, loading; action commits on release inside the control)
✅ - IconButton (icon-only button, 44px target, accessible label)
✅ - Input (Apple-style; filled / bordered / plain; focus ring; placeholder; caret typing)
✅ - Scrollbar
Dropdown
✅ - Icons -> The icons will be basically SVGs which user can add by <SVGName /> which will load from the Assets/RoboCountry.pack and RoboIcons.pack.
Menu
Context Menu
✅ - SVGs -> SVGs will be directly supported in XML user can write <svg><path> etc</svg> they can write the SVg directly in XML how they write the components. They can use the icons too as <IconName />
Ellipse/Circle
Rectangle
Polygon
Star
✅ - Flexbox
✅ - Grid
✅ - Position
Checkbox
Radio
Toggle
Slider
List
Table
✅ - BrandLogo
✅ - TitleBar (also Navbar; 57px, background `color.bg.secondary`, BrandLogo lockup on the left, outline sidebar IconButton beside it, split ThemeIconToggle on the right)
✅ - Sidebar (also Pane; 282px, `color.bg.secondary`). Two names: **Options** (`variant="options"`) is icon+text and collapses to an icon rail, with no inner panel. **Activity** (`variant="activity"`) keeps the icon rail and opens a wider panel for the selected SidebarItem. Ctrl+B or Command+B shows or hides it.
✅ - SidebarItem (also PaneItem, PaneItems; icon and label; an Options button, or an Activity that reveals its child items in the wide panel; `id` names the page a Container `when` shows; `selected="true"` opens that page; `section="true"` is a group label; hover, press, keyboard focus ring, disabled, and loading like a button; Tab moves to it, arrows move within its sidebar, Enter or Space activates; can sit in a grid, flex, or position)
✅ - Separator (also Divider; 1px foundation rule, optional label, solid / subtle / dashed / dotted / fade)
ThemeToggle
ThemeIconToggle
✅ - Shape (rect, ellipse, or path). Theme pairs (`darkX`, `darkY`, `darkBottom`, `darkFill`, `darkOpacity`, `darkScale`, `darkRotate`), `transition="0.5s cubic-bezier(0, -0.02, 0.4, 1.25)"`, hover (`hoverX`, `darkHoverX`, `hoverY`, `hoverScale`, `hoverRotate`, `hoverFill`, `hoverWithin`), `clip`, and `toggle="theme"` animate and transition any shape. `animation="name"` plus `animationDelay` plays a Keyframes clip.
✅ - Keyframes (name, duration, easing `linear` / `ease` / `ease-in` / `ease-out` / `ease-in-out` / `cubic-bezier(x1, y1, x2, y2)`, `iteration="infinite"`, optional delay). Each Keyframe has `at` from 0 to 1 and any of `x`, `y`, `opacity`, `scale`, `rotate`. Use these to animate and transition shapes. The day/night switch above Theme icon in Demo.xml is built from Shape and Keyframes.
✅ - Desktop (the OS shell). It holds a Background, a TopBarMenu, an AppShell, and a Dock. The page content goes inside AppShell and still paints as damage rectangles. The shell page lives in the OS at RoboDesktop/Desktop.xml. The compiler turns it into C++ for the RoboticelaOS platform.
✅ - TopBarMenu (30px, full width, above the wallpaper). A vector Apple mark, the semibold app name (`app`, Finder by default), regular menu labels, and the status icons. `clock` is the date and time. TopBarMenuItem is one menu label. The open item is `#007AFF` with white text. A click opens that menu and dirties only the bar.
✅ - Background (`color` or `fill`, a foundation token such as `{color.bg.primary}`). Fills the shell behind the app.
✅ - AppShell (`title`). The bar above the app. Its children are the page: headings, buttons, flex, and the other components.
✅ - Dock and DockItem (`icon`, `label`, `selected="true"`). The dock sits on the bottom. A click selects that item and dirties only the dock.


Use the current which already exists as folders to make everything and put everything correctly in these structure. I want to build first for the web so build it for the web platform first but the implementation must be independant of everything so we will just need to put implementation in RoboPlatforms and must not need any other changes to the code. Use the WebAssembly for web and on request of the page the HTMl will be provided too which will be made of the XML so we can get the best SEO.

Programmer writes XML. The compiler turns that XML into C++. That C++ is compiled for the platform. There is no XML interpreter and no IDE. Demo.xml is the page. Run it in the browser with RunDemo.ps1. DevKit/DevKit.xml is the component app. Run it with RunDevKit.ps1. RoboPlatforms/RoboticelaOS is the native platform: Present copies one damage rectangle into the framebuffer at 1:1. The OS shell is RoboDesktop/Desktop.xml, compiled with RoboCompileDesktop.ps1. A root `padding` attribute sets the page inset; DevKit uses `0` so the TitleBar meets the window edge.