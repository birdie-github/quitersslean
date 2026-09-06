# Feed-only article viewer

This stage retains QtWebKit for article layout and newspaper DOM updates. It removes
website tabs, address/navigation controls, the ad blocker, browser selection,
JavaScript controls, browser cookie/cache preferences, and page-to-database capture.
Existing database content and legacy database columns are retained.

Dependencies: Qt 5.15.x, QtWebKit 5.212, SQLite, and libxml2 development headers
(discovered through pkg-config). On Fedora the new build dependency is libxml2-devel.

Article HTML is parsed by libxml2 with network access disabled, then serialized
through an element/attribute allowlist before reaching WebKit. Scripts, stylesheets,
frames, objects, forms' interactive controls, SVG and MathML are excluded.
Embedded audio/video/frame source URLs become explicit external links. A small
set of inline text styles is retained. The application's article stylesheet remains
user configurable. Original stored HTML is never overwritten by normalization.

Global/per-feed image preferences remain effective in single-article and newspaper
views. Publisher images use a separate network manager: no feed credentials,
cookies, Referer, disk cache or SSL-error exceptions. Redirects are limited to five,
restricted to HTTP(S), and may not downgrade HTTPS. Remote responses are buffered
(up to 16 MiB), checked as raster images (PNG/JPEG/GIF/WebP/BMP, up to 64 megapixels),
then returned to WebKit. Requests time out after 30 seconds. Qt image handlers are
needed for the corresponding formats. Inline raster data URLs are limited to 8 MiB.
Article input is limited to 4 MiB; HTML traversal is limited to 128 levels and 50,000
nodes. Very large or unusual content should be opened externally.

This is a resource policy, not an OS sandbox. Permitted image loading still contacts
publisher servers, and decoding still uses native image libraries. Remote images
are not forced off for existing users. Feed fetching/authentication and the explicit
download manager retain their own existing behavior and settings.

Web links open through the OS default browser. Mail links use the default mail
handler. Feed-provided custom protocols and local-file links are not dispatched.
Internal article anchors scroll within the pane. Only application-generated
newspaper controls may use the quiterss action scheme.

Manual checks (no build was run while producing this patch):
- Select articles whose old per-feed settings requested full pages or JavaScript;
  the pane must show only stored article content.
- Exercise single-article and newspaper views, read/star/label/delete controls,
  search highlighting, selection/copy, zoom, print/preview and reader tabs.
- Toggle images in both layouts, including inherited/per-feed overrides.
- Check a redirected image, a WebP and a GIF with the installed Qt image handlers.
- Confirm ordinary, middle-clicked and context-menu links open externally, while
  in-article anchors stay local. Test an enclosure's explicit Save Link action.
- Try HTML with script, iframe, meta refresh, object, file URLs, CSS URLs and forged
  quiterss action links: it must neither navigate nor execute application actions.
- Confirm old archived HTML remains in the database after viewing it.
