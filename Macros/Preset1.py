#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# find source
clip = FindSource('Clip')
SetActiveSource(clip)

# Properties modified on clip
clip.Origin = [0.0, 0.0, 0.0]
clip.Normal = [0.0, 1.0, 0.0]

# find source
contour = FindSource('Contour')
SetActiveSource(contour)

# Properties modified on contour
contour.ContourField = 'Density'
contour.Isosurfaces = [0.738, 0.7392, 0.7404, 0.7416, 0.7428]
contour.ColorField = 'Density'
contour.ColorPalette = 'Cool to Warm'
contour.ColorRange = [0.7377, 0.7428]

# find source
slice = FindSource('Slice')
SetActiveSource(slice)

# Properties modified on slice
slice.NumberOfPlanes = 5
slice.Spacing = 3.0
slice.Normal = [1.0, 0.0, 0.0]
slice.ColorField = 'Density'
slice.ColorPalette = 'Cool to Warm'
slice.ColorRange = [0.695, 0.7385]

# Properties modified on slice
slice.Origin = [3.0, 0.0, 0.0]

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).