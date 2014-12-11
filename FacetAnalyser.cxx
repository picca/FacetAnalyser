
#include "FacetAnalyser.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <vtkSmartPointer.h>

#include <vtkMath.h>
#include <vtkPolyDataNormals.h>
#include <vtkMeshQuality.h>
#include <vtkGaussianSplatter.h>
#include <vtkImageCast.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPolyData.h>
#include <vtkIdTypeArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>

#include <itkVTKImageToImageFilter.h>
#include <itkShiftScaleImageFilter.h>
#include <itkHMinimaImageFilter.h>
#include <itkRegionalMinimaImageFilter.h>
#include <itkConnectedComponentImageFilter.h>
#include <itkMorphologicalWatershedFromMarkersImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkAddImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkChangeLabelImageFilter.h>
#include <itkMaskImageFilter.h>

#include <itkStatisticsLabelObject.h>
#include <itkLabelMap.h>
#include <itkLabelImageToStatisticsLabelMapFilter.h>

#define SMB 1.1 //splatter boundaries: make it bigger than 1 to catch the centres!
#define msigma 2 //render gauss upto msigma*sigma; 1~68%; 2~95%; 3~99%


vtkStandardNewMacro(FacetAnalyser);

//----------------------------------------------------------------------------
// Description:
FacetAnalyser::FacetAnalyser(){
    //this->SetNumberOfOutputPorts(2);

    this->SampleSize= 101;
    this->AngleUncertainty= 10;
    this->MinTrianglesPerFacet= 10;
    }

//----------------------------------------------------------------------------

int FacetAnalyser::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
    {
    // get the info objects
    vtkInformation *inInfo0 = inputVector[0]->GetInformationObject(0);
    //vtkInformation *inInfo1 = inputVector[0]->GetInformationObject(1);
    vtkInformation *outInfo0 = outputVector->GetInformationObject(0);
    //vtkInformation *outInfo1 = outputVector->GetInformationObject(1);

    // get the input and ouptut
    vtkPolyData *input = vtkPolyData::SafeDownCast(
        inInfo0->Get(vtkDataObject::DATA_OBJECT()));
    //vtkImplicitFunction *inputIF = vtkImplicitFunction::SafeDownCast(inInfo1->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData *output = vtkPolyData::SafeDownCast(
        outInfo0->Get(vtkDataObject::DATA_OBJECT()));


    double da= this->AngleUncertainty / 180.0 * vtkMath::Pi(); 
    double f= 1/2./sin(da)/sin(da); //sin(da) corresponds to sigma
    double R= msigma / double(SMB) * sqrt(1/2./f);

    vtkSmartPointer<vtkPolyDataNormals> PDnormals0= vtkSmartPointer<vtkPolyDataNormals>::New();
    PDnormals0->SetInputData(input);
    PDnormals0->ComputePointNormalsOff(); 
    PDnormals0->ComputeCellNormalsOn();
    PDnormals0->Update();

    vtkSmartPointer<vtkMeshQuality> cellArea= vtkSmartPointer<vtkMeshQuality>::New();
    cellArea->SetInputConnection(PDnormals0->GetOutputPort());
    cellArea->SetTriangleQualityMeasureToArea();
    cellArea->SaveCellQualityOn();
    cellArea->Update();

    vtkDataArray *normals= PDnormals0->GetOutput()->GetCellData()->GetNormals();
    vtkDataArray *areas= cellArea->GetOutput()->GetCellData()->GetArray("Quality");

    ////regard normals coords as point coords
    vtkSmartPointer<vtkPoints> Points = vtkSmartPointer<vtkPoints>::New();
    vtkIdType ndp= normals->GetNumberOfTuples();
    for(vtkIdType i= 0; i < ndp; i++)
        Points->InsertNextPoint(normals->GetTuple3(i));

    vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
    polydata->SetPoints(Points);
    polydata->GetPointData()->SetScalars(areas);

    vtkSmartPointer<vtkGaussianSplatter> Splatter = vtkSmartPointer<vtkGaussianSplatter>::New();
    Splatter->SetInputData(polydata);
    Splatter->SetSampleDimensions(this->SampleSize,this->SampleSize,this->SampleSize); //set the resolution of the final! volume
    Splatter->SetModelBounds(-SMB,SMB, -SMB,SMB, -SMB,SMB);
    Splatter->SetExponentFactor(-f); //GaussianSplat decay value
    Splatter->SetRadius(R); //GaussianSplat truncated outside Radius
    Splatter->SetAccumulationModeToSum();
    Splatter->ScalarWarpingOn(); //use individual weights
    Splatter->CappingOff(); //don't pad with 0
 
    vtkSmartPointer<vtkImageCast> cast = vtkSmartPointer<vtkImageCast>::New();
    cast->SetInputConnection(Splatter->GetOutputPort());
    cast->SetOutputScalarTypeToDouble();
    cast->Update(); //seems to be essential for vtk-6.1.0 + itk-4.5.1


/////////////////////////////////////////////////
///////////////// going to ITK //////////////////
/////////////////////////////////////////////////


    const int dim = 3;

    typedef double PixelType; //vtk 6.1.0 + itk 4.5.1 seem to use double instead of float
    typedef itk::Image< PixelType, dim >    ImageType;
    typedef ImageType GreyImageType;

    typedef itk::VTKImageToImageFilter<ImageType> ConnectorType;

    ConnectorType::Pointer vtkitkf = ConnectorType::New();
    vtkitkf->SetInput(cast->GetOutput()); //NOT GetOutputPort()!!!
    vtkitkf->Update();

/////////////////////// creating the sphere mask
//also outer radius to ensure proper probability interpretation!!!

    typedef  unsigned char MPixelType;

    const unsigned int Dimension = dim;

    typedef itk::Image<MPixelType,  Dimension>   MImageType;


    typedef itk::ShiftScaleImageFilter<GreyImageType, GreyImageType> SSType;
    SSType::Pointer ss = SSType::New();
    ss->SetScale(-1); //invert by mul. with -1
    ss->SetInput(vtkitkf->GetOutput());
    ss->Update();

    typedef unsigned short LabelType;
    typedef itk::Image<LabelType,  dim>   LImageType;

    //use ws from markers because the label image is needed twice
    bool ws1_conn= true;  //fully connected borders!
    bool ws2_conn= false; //finer borders??? and no lost labels! (see mws pdf)
    bool ws3_conn= ws2_conn; 

    typedef itk::HMinimaImageFilter<GreyImageType, GreyImageType> HMType; //seems for hmin in-type==out-type!!!
    HMType::Pointer hm= HMType::New();
    hm->SetHeight(1 / double(ndp) * this->MinTrianglesPerFacet);
    hm->SetFullyConnected(ws1_conn);
    hm->SetInput(ss->GetOutput());

    typedef itk::RegionalMinimaImageFilter<GreyImageType, MImageType> RegMinType;
    RegMinType::Pointer rm = RegMinType::New();
    rm->SetFullyConnected(ws1_conn);
    rm->SetInput(hm->GetOutput());

    // connected component labelling
    typedef itk::ConnectedComponentImageFilter<MImageType, LImageType> CCType;
    CCType::Pointer labeller = CCType::New();
    labeller->SetFullyConnected(ws1_conn);
    labeller->SetInput(rm->GetOutput());

    typedef itk::MorphologicalWatershedFromMarkersImageFilter<GreyImageType, LImageType> MWatershedType;
    MWatershedType::Pointer ws1 = MWatershedType::New();
    ws1->SetMarkWatershedLine(true); //use borders as marker in sd. ws
    ws1->SetFullyConnected(ws1_conn); //true reduces amount of watersheds
    //ws1->SetLevel(1 / double(ndp) * atof(argv[4])); //if 0: hminima skipted?
    ws1->SetInput(ss->GetOutput());
    ws1->SetMarkerImage(labeller->GetOutput());
    ws1->Update(); //whith out this update label one is lost for facet_holger particle0195!!! Why???

    // extract the watershed lines and combine with the orginal markers
    typedef itk::BinaryThresholdImageFilter<LImageType, LImageType> ThreshType;
    ThreshType::Pointer th = ThreshType::New();
    th->SetUpperThreshold(0);
    th->SetOutsideValue(0);
    // set the inside value to the number of markers + 1
    th->SetInsideValue(labeller->GetObjectCount() + 1);
    th->SetInput(ws1->GetOutput());

    // Add the marker image to the watershed line image
    typedef itk::AddImageFilter<LImageType, LImageType, LImageType> AddType;
    AddType::Pointer adder1= AddType::New();
    adder1->SetInput1(th->GetOutput());
    adder1->SetInput2(labeller->GetOutput());

    // compute a gradient
    typedef itk::GradientMagnitudeImageFilter<GreyImageType, GreyImageType> GMType;
    GMType::Pointer gm1= GMType::New();
    gm1->SetInput(ss->GetOutput());

    // Now apply sd. watershed
    MWatershedType::Pointer ws2 = MWatershedType::New();
    ws2->SetMarkWatershedLine(false); //no use for a border in sd. stage
    ws2->SetFullyConnected(ws2_conn); 
    //ws->SetLevel(1 / double(ndp) * atof(argv[4])); //if 0: hminima skipted?
    ws2->SetInput(gm1->GetOutput());
    ws2->SetMarkerImage(adder1->GetOutput());

    // delete the background label
    typedef itk::ChangeLabelImageFilter<LImageType, LImageType> ChangeLabType;
    ChangeLabType::Pointer ch1= ChangeLabType::New();
    ch1->SetInput(ws2->GetOutput());
    ch1->SetChange(labeller->GetObjectCount() + 1, 0);

    // combine the markers again
    //this result is not the same as ws2 because the bg-label has grown!
    AddType::Pointer adder2 = AddType::New();
    adder2->SetInput1(th->GetOutput());
    adder2->SetInput2(ch1->GetOutput());

    GMType::Pointer gm2 = GMType::New();
    gm2->SetInput(gm1->GetOutput());

    MWatershedType::Pointer ws3 = MWatershedType::New();
    ws3->SetFullyConnected(ws3_conn);
    ws3->SetInput(gm2->GetOutput());
    ws3->SetMarkerImage(adder2->GetOutput());
    ws3->SetMarkWatershedLine(false);			

    // delete the background label again
    ChangeLabType::Pointer ch2= ChangeLabType::New();
    ch2->SetInput(ws3->GetOutput());
    ch2->SetChange(labeller->GetObjectCount() + 1, 0);

////////////////////////Now label and grow the facet reagions... done.


    ////spalter only single points with weights    
    vtkGaussianSplatter *Splatter2 = vtkGaussianSplatter::New();
    Splatter2->SetInputData(polydata);
    Splatter2->SetSampleDimensions(this->SampleSize,this->SampleSize,this->SampleSize); //set the resolution of the final! volume
    Splatter2->SetModelBounds(-SMB,SMB, -SMB,SMB, -SMB,SMB);
    Splatter2->SetExponentFactor(-1); //GaussianSplat decay value
    Splatter2->SetRadius(0); //only splat single points
    Splatter2->SetAccumulationModeToSum();
    Splatter2->ScalarWarpingOn(); //use individual weights
    Splatter2->CappingOff(); //don't pad with 0
    //Splatter->SetScaleFactor(atof(argv[4]));//scale comes from each point
    //FilterWatcher watcher(Splatter, "filter");
    //Splatter->Update();

    vtkImageCast* cast2 = vtkImageCast::New();
    cast2->SetInputConnection(Splatter2->GetOutputPort());
    //cast->SetOutputScalarTypeToUnsignedChar();
    //cast2->SetOutputScalarTypeToFloat();
    cast2->SetOutputScalarTypeToDouble();
    cast2->Update(); //seems to be essential for vtk-6.1.0 + itk-4.5.1

    ConnectorType::Pointer vtkitkf2 = ConnectorType::New();
    vtkitkf2->SetInput(cast2->GetOutput()); //NOT GetOutputPort()!!!
    vtkitkf2->Update();

    typedef itk::MaskImageFilter<LImageType, GreyImageType, LImageType> MaskType2;
    MaskType2::Pointer mask2 = MaskType2::New();
    mask2->SetInput1(ch2->GetOutput());
    mask2->SetInput2(vtkitkf2->GetOutput()); //mask

    typedef itk::StatisticsLabelObject< LabelType, dim > LabelObjectType;
    typedef itk::LabelMap< LabelObjectType > LabelMapType;

    typedef itk::LabelImageToStatisticsLabelMapFilter<LImageType, GreyImageType, LabelMapType> ConverterType;
    ConverterType::Pointer converter = ConverterType::New();
    converter->SetInput(mask2->GetOutput());
    converter->SetFeatureImage(vtkitkf2->GetOutput()); //this should be the single splat grey image to be exact!
    //converter->SetFullyConnected(true); //true: 26-connectivity; false: 6-connectivity
    converter->SetComputePerimeter(false);
    converter->Update();

    vtkPoints* fpoints = vtkPoints::New();
    vtkFloatArray* weights = vtkFloatArray::New();
    weights->SetNumberOfComponents(1);
    weights->SetName ("weight");

    FILE *off;
    //static const char*  fn= "test";
    static const std::string fn= "test";

    off= fopen ((fn + ".fdat").data(),"w");
    fprintf(off, "#i\tfn_x\tfn_y\tfn_z\trel_facet_size\tabs_facet_size\n"); //local max value
    double c_x, c_y, c_z, fw, tfw= 0;
 
    LabelMapType::Pointer labelMap = converter->GetOutput();
    const LabelObjectType * labelObject;
    for( unsigned int label=1; label<=labelMap->GetNumberOfLabelObjects(); label++ )
        {
        try{
            labelObject = labelMap->GetLabelObject( label );
            }
        catch( itk::ExceptionObject exp ){
            if (strstr(exp.GetDescription(), "No label object with label")){
                //std::cerr << exp.GetDescription() << std::endl;
                std::cout << "Missing label: " << label << std::endl;
                continue;
                }
            }

/*******************
Since GetCenterOfGravity(), GetCentroid() are in 3D the centres can lie off the unit sphere!
The inverse of the length of the centre vector is therefor a measure of how concentrated/dispersed the label is and therefor how destinct a facet is
*/
//         c_x= labelObject->GetCentroid()[0];
//         c_y= labelObject->GetCentroid()[1];
//         c_z= labelObject->GetCentroid()[2];
        c_x= labelObject->GetCenterOfGravity()[0];
        c_y= labelObject->GetCenterOfGravity()[1];
        c_z= labelObject->GetCenterOfGravity()[2];
        fw= labelObject->GetSum();
        //fprintf(off, "%d\t%f\t%f\t%f\t%f\t%f\n",label, c_x, c_y, c_z, (labelObject->GetMaximum()) / labelObject->GetSize(), fw / ts); //GetSize(), GetVariance()
        fprintf(off, "%d\t%f\t%f\t%f\t%f\t%f\n",label, c_x, c_y, c_z, fw, fw); //GetSize(), GetVariance()
        fpoints->InsertNextPoint(c_x, c_y, c_z);
        weights->InsertNextValue(fw); //InsertNextTuple1
        tfw+= fw;
        //fpoints->SetScalar(fw); //not possible!
        }
    fclose(off);

    //unsigned int nfp= fpoints->GetNumberOfPoints();
    vtkIdType nfp= fpoints->GetNumberOfPoints();

    //fstream of;
    //of.open((fn + ".adat").data(), ios::out);
    //of << "#p0_x\tp0_y\tp0_z\tp1_x\tp1_y\tp1_z\ta" << endl;
    FILE *of;
    of= fopen ((fn + ".adat").data(),"w");
    fprintf(of, "#i1\ti2\tp0_x\tp0_y\tp0_z\tp1_x\tp1_y\tp1_z\tangle\ta_weight\n");

    if(nfp > 1){

        double angle, aw;
        double p0[3], p1[3];
        //unsigned int u,v;
        vtkIdType u,v;

        for(u= 0; u < nfp - 1 ; u++){
            for(v= u + 1; v < nfp; v++){
                fpoints->GetPoint(u, p0); 
                fpoints->GetPoint(v, p1);
                vtkMath::Normalize(p0);
                vtkMath::Normalize(p1);
                angle= acos(vtkMath::Dot(p0, p1)) * 180.0 / vtkMath::Pi();
                aw= 2 / (1/weights->GetTuple1(u) + 1/weights->GetTuple1(v)); //harmonic mean because its the "smallest" of the Pythagorean means: http://en.wikipedia.org/wiki/Pythagorean_means
                //cout << angle << "; ";
                fprintf(of, "%lld\t%lld\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", u+1 , v+1, p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], angle, aw);
                }
            }

        //cout << endl;
        }
    //of.close();
    fclose(of);



///////////probe each splat point value ;-)))
    vtkPoints* tps = vtkPoints::New();
    tps= polydata->GetPoints();

    vtkDataArray *tvs=NULL;
    //tvs= vtkDoubleArray::SafeDownCast(Splatter->GetOutput()->GetPointData()->GetScalars()); //triangel values
    tvs= Splatter->GetOutput()->GetPointData()->GetScalars(); //triangel values

//     LImageType tls;
//     tls= mask2->GetOutput(); //triangle facte labels   Splatter->GetOutput()->GetPointData()->GetScalars();

    vtkIdType nt, k, idx, pi[3];
    //unsigned int idx;
    LImageType::IndexType lidx;
    //nt= polydata->GetPointData()->GetNumberOfPoints();
    nt= tps->GetNumberOfPoints();

    FILE *tf;
    tf= fopen ((fn + ".tdat").data(),"w");
    fprintf(tf, "#i\tlabel\tvalue\n");


    vtkSmartPointer<vtkIdTypeArray> fId= vtkSmartPointer<vtkIdTypeArray>::New();
    fId->SetName("FacetIds");
    fId->SetNumberOfComponents(1);

    vtkSmartPointer<vtkDoubleArray> fPb= vtkSmartPointer<vtkDoubleArray>::New();
    fPb->SetName("FacetProbabilities");
    fPb->SetNumberOfComponents(1);

    LabelType tl;
    double pp[3], tv;
    for(k= 0; k < nt; k++){
        tps->GetPoint(k, pp); 
        idx= Splatter->ProbePoint(pp, pi);
        lidx[0]= pi[0]; //this is hard coding the dim to equal 3!!!
        lidx[1]= pi[1]; //this is hard coding the dim to equal 3!!!
        lidx[2]= pi[2]; //this is hard coding the dim to equal 3!!!
        tv= *tvs->GetTuple(idx);
        //tl= tls->GetPixel(IndexType(idx));
        tl= mask2->GetOutput()->GetPixel(lidx);

        if (idx < 0){
            vtkErrorMacro(<< "Prope Point: " << pp[0] << ";" << pp[1] << ";" << pp[2] << " not insied sample data");
            return VTK_ERROR;
            }

        //std::cerr << tl << std::endl;

        fId->InsertNextValue(tl);
        fPb->InsertNextValue(tv);

        }
    fclose(tf);

    // Copy original points and point data
    output->CopyStructure( input );
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    output->GetCellData()->AddArray(fId);
    output->GetCellData()->AddArray(fPb);



    return 1;
    }

//----------------------------------------------------------------------------
void FacetAnalyser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SampleSize: " << this->SampleSize << endl;
  os << indent << "AngleUncertainty: " << this->AngleUncertainty << endl;
  os << indent << "MinTrianglesPerFacet: " << this->MinTrianglesPerFacet << endl;
  os << indent << endl;
}
