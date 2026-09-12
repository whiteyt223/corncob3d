# Owner publication and maintenance

Public source repository: https://github.com/whiteyt223/corncob3d

The initial source snapshot was uploaded through the owner's connected GitHub account. The old private `corncob3d-source-port` repository was not modified or made public: it contains the earlier prototype and original music in its history.

Push reviewed source changes to this repository. The GitHub Actions workflow builds/tests them and produces a Cloudflare Pages artifact. It does not deploy or contain Cloudflare credentials. Upload the built `dist` contents, including `_headers`, to the existing Cloudflare Pages project after reviewing the build result.

The separate `corncob3d-github-link.zip` already contains the source footer link and prior security/menu fixes. Website publication is still an owner action.

`publish-to-github.ps1` is retained only for the original clean-ZIP creation workflow; it intentionally refuses to overwrite this now-existing repository. Normal updates should use Git commits/pushes or pull requests.

Private vulnerability reporting and branch protection are repository settings for the owner to configure. Their enabled state has not been asserted by this upload.
