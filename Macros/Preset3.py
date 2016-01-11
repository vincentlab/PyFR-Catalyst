#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# find source
clip = FindSource('Clip')
SetActiveSource(clip)

# Properties modified on clip
clip.Origin = [10.0, 10.0, 10.0]
clip.Normal = [0.0, 1.0, 0.0]

# find source
contour = FindSource('Contour')
SetActiveSource(contour)

contour.ContourField = 'Density'
contour.Isosurfaces = [0.7197786, 0.717, 0.7215]
contour.ColorField = 'Velocity_u'
contour.ColorPalette = 'Cool to Warm'
contour.ColorRange = [-0.25, 0.5]

# find source
slice = FindSource('Slice')
SetActiveSource(slice)

# Properties modified on slice
slice.NumberOfPlanes = 0

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).